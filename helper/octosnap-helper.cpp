// octosnap-helper — privileged, allow-listed operations for OctoSnap.
//
// Runs as root through doas/sudo. It accepts no shell input: every external
// command runs through execv() with an argv vector and fixed absolute paths.
//
// Usage:
//   octosnap-helper list
//   octosnap-helper create  <name> <mountpoint>
//   octosnap-helper destroy <name> [--remove-backing]
//   octosnap-helper mount-ro <name> <dir>
//   octosnap-helper umount  <dir>
//   octosnap-helper restore <name> [--rename-existing] <relpath>...
//
// restore copies items from the snapshot's read-only mount back to the same
// relative path on the live filesystem. Every path component is opened with
// openat(O_NOFOLLOW) from a descriptor of the mount roots, so symlinks on the
// live filesystem cannot redirect writes made as root, and nothing crosses
// into another filesystem. Files are written to a temporary name and then
// renamed into place, keeping owner, mode and timestamps from the snapshot.

#include <sys/param.h>
#include <sys/stat.h>
#if defined(__NetBSD__)
#include <sys/statvfs.h>
#else
#include <sys/mount.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <set>
#include <string>
#include <vector>

namespace {

const char *kFssconfig  = "/sbin/fssconfig";
const char *kMount      = "/sbin/mount";
const char *kUmount     = "/sbin/umount";
const char *kStateDir   = "/var/db/octosnap";
const char *kBackingDir = "/var/db/octosnap/backing";
const char *kMetaFile   = "/var/db/octosnap/snapshots.tsv";
const int kMaxUnits     = 16;

struct Entry {
    std::string name;
    std::string unit;
    std::string mountpoint;
    std::string backing;
    long long created = 0;
    std::string rodir;   // empty when not mounted read-only by OctoSnap
};

[[noreturn]] void fail(const std::string &msg, int code = 1) {
    std::fprintf(stderr, "octosnap-helper: %s\n", msg.c_str());
    std::exit(code);
}

[[noreturn]] void usage() {
    fail("usage: list | create <name> <mp> | destroy <name> [--remove-backing] | "
         "mount-ro <name> <dir> | umount <dir> | "
         "restore <name> [--rename-existing] <relpath>...", 2);
}

bool validName(const std::string &s) {
    if (s.empty() || s.size() > 64 || s[0] == '.') return false;
    for (unsigned char c : s)
        if (!(std::isalnum(c) || c == '.' || c == '_' || c == '-')) return false;
    return true;
}

bool validUnit(const std::string &s) {
    if (s.size() < 4 || s.compare(0, 3, "fss") != 0) return false;
    for (size_t i = 3; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}

bool validAbsPath(const std::string &p) {
    if (p.empty() || p[0] != '/' || p.size() >= PATH_MAX) return false;
    if (p.find("/../") != std::string::npos) return false;
    if (p.size() >= 3 && p.compare(p.size() - 3, 3, "/..") == 0) return false;
    for (char c : p)
        if (c == '\t' || c == '\n') return false;
    return true;
}

std::string realPath(const std::string &p) {
    char buf[PATH_MAX];
    if (realpath(p.c_str(), buf) == nullptr) fail("cannot resolve path: " + p);
    return buf;
}

bool isFfsMountPoint(const std::string &mp) {
#if defined(__NetBSD__)
    struct statvfs *mnts = nullptr;
    const int n = getmntinfo(&mnts, ST_NOWAIT);
    for (int i = 0; i < n; ++i)
        if (mp == mnts[i].f_mntonname && std::strcmp(mnts[i].f_fstypename, "ffs") == 0)
            return true;
    return false;
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__) || defined(__APPLE__)
    struct statfs *mnts = nullptr;
    const int n = getmntinfo(&mnts, MNT_NOWAIT);
    for (int i = 0; i < n; ++i)
        if (mp == mnts[i].f_mntonname && (std::strcmp(mnts[i].f_fstypename, "ufs") == 0 || std::strcmp(mnts[i].f_fstypename, "ffs") == 0))
            return true;
    return false;
#else
    (void)mp;
    return true;
#endif
}

// True when dir is mounted from the given device (e.g. /dev/fss0).
bool isMountedFrom(const std::string &dir, const std::string &dev) {
#if defined(__NetBSD__)
    struct statvfs *mnts = nullptr;
    const int n = getmntinfo(&mnts, ST_NOWAIT);
    for (int i = 0; i < n; ++i)
        if (dir == mnts[i].f_mntonname && dev == mnts[i].f_mntfromname) return true;
    return false;
#else
    (void)dir;
    (void)dev;
    return true;
#endif
}

bool isEmptyDir(const std::string &d) {
    DIR *dp = opendir(d.c_str());
    if (!dp) return false;
    bool empty = true;
    while (struct dirent *de = readdir(dp)) {
        if (std::strcmp(de->d_name, ".") != 0 && std::strcmp(de->d_name, "..") != 0) {
            empty = false;
            break;
        }
    }
    closedir(dp);
    return empty;
}

// Runs a command with an argv vector (no shell). Optionally captures stdout.
int runCmd(const std::vector<std::string> &args, std::string *out) {
    int fds[2] = {-1, -1};
    if (out && pipe(fds) != 0) return -1;

    const pid_t pid = fork();
    if (pid < 0) return -1;

    if (pid == 0) {
        if (out) {
            dup2(fds[1], STDOUT_FILENO);
            close(fds[0]);
            close(fds[1]);
        }
        std::vector<char *> argv;
        argv.reserve(args.size() + 1);
        for (const auto &a : args) argv.push_back(const_cast<char *>(a.c_str()));
        argv.push_back(nullptr);
        execv(argv[0], argv.data());
        _exit(127);
    }

    if (out) {
        close(fds[1]);
        char buf[4096];
        ssize_t n;
        while ((n = read(fds[0], buf, sizeof buf)) > 0) out->append(buf, static_cast<size_t>(n));
        close(fds[0]);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

void ensureStateDirs() {
    if (mkdir(kStateDir, 0700) != 0 && errno != EEXIST) fail("cannot create state directory");
    if (mkdir(kBackingDir, 0700) != 0 && errno != EEXIST) fail("cannot create backing directory");
}

std::vector<Entry> loadMeta() {
    std::vector<Entry> v;
    FILE *f = std::fopen(kMetaFile, "r");
    if (!f) return v;

    char line[4096];
    while (std::fgets(line, sizeof line, f)) {
        std::string s(line);
        if (!s.empty() && s.back() == '\n') s.pop_back();

        std::vector<std::string> cols;
        size_t start = 0;
        for (;;) {
            const size_t t = s.find('\t', start);
            if (t == std::string::npos) {
                cols.push_back(s.substr(start));
                break;
            }
            cols.push_back(s.substr(start, t - start));
            start = t + 1;
        }
        if (cols.size() < 5) continue;

        Entry e;
        e.name = cols[0];
        e.unit = cols[1];
        e.mountpoint = cols[2];
        e.backing = cols[3];
        e.created = std::atoll(cols[4].c_str());
        if (cols.size() > 5) e.rodir = cols[5];
        v.push_back(e);
    }
    std::fclose(f);
    return v;
}

void saveMeta(const std::vector<Entry> &v) {
    const std::string tmp = std::string(kMetaFile) + ".tmp";
    const int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) fail("cannot write metadata");

    std::string out;
    for (const auto &e : v) {
        out += e.name + '\t' + e.unit + '\t' + e.mountpoint + '\t' + e.backing + '\t' +
               std::to_string(e.created) + '\t' + e.rodir + '\n';
    }
    if (write(fd, out.data(), out.size()) != static_cast<ssize_t>(out.size()))
        fail("short write to metadata");
    fsync(fd);
    close(fd);

    if (rename(tmp.c_str(), kMetaFile) != 0) fail("cannot replace metadata");
}

// Units currently configured in the kernel, per fssconfig -l.
std::set<std::string> activeUnits() {
    std::string out;
    if (runCmd({kFssconfig, "-l"}, &out) != 0) fail("fssconfig -l failed");

    std::set<std::string> used;
    size_t pos = 0;
    while (pos < out.size()) {
        const size_t nl = out.find('\n', pos);
        const std::string line = out.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        pos = (nl == std::string::npos) ? out.size() : nl + 1;

        const std::string tok = line.substr(0, line.find_first_of(" \t"));
        if (validUnit(tok)) used.insert(tok);
    }
    return used;
}

std::string freeUnit() {
    const auto used = activeUnits();
    for (int i = 0; i < kMaxUnits; ++i) {
        const std::string u = "fss" + std::to_string(i);
        if (used.count(u) == 0) return u;
    }
    fail("no free fss unit");
}

void cmdList() {
    const auto meta = loadMeta();
    const auto active = activeUnits();
    for (const auto &e : meta) {
        // st_blocks is in 512-byte units on NetBSD (POSIX): allocated size, not apparent size.
        struct stat sb {};
        const long long used =
            (stat(e.backing.c_str(), &sb) == 0) ? static_cast<long long>(sb.st_blocks) * 512LL : 0LL;
        std::printf("%s\t%s\t%s\t%s\t%lld\t%s\t%d\t%lld\n",
                    e.name.c_str(), e.unit.c_str(), e.mountpoint.c_str(), e.backing.c_str(),
                    e.created, e.rodir.c_str(), active.count(e.unit) ? 1 : 0, used);
    }
}

void cmdCreate(const std::string &name, const std::string &mpArg) {
    if (!validName(name)) fail("invalid snapshot name");
    if (!validAbsPath(mpArg)) fail("invalid mount point");

    const std::string mp = realPath(mpArg);
    if (!isFfsMountPoint(mp)) fail("not a mounted FFS filesystem: " + mp);

    ensureStateDirs();

    for (const auto &e : loadMeta())
        if (e.name == name) fail("snapshot already exists: " + name);

    // The backing store must live on a different filesystem than the one
    // being snapshotted, otherwise copy-on-write writes recurse into the snapshot.
    const std::string backingDir = realPath(kBackingDir);
    struct stat sbMp {}, sbBk {};
    if (stat(mp.c_str(), &sbMp) != 0 || stat(backingDir.c_str(), &sbBk) != 0)
        fail("stat failed");
    if (sbMp.st_dev == sbBk.st_dev)
        fail("backing store must reside on a different filesystem than " + mp);

    const std::string backing = std::string(kBackingDir) + "/" + name + ".fss";
    const int fd = open(backing.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd < 0) fail("cannot create backing file: " + backing);
    close(fd);

    const std::string unit = freeUnit();
    if (runCmd({kFssconfig, "-c", unit, mp, backing}, nullptr) != 0) {
        unlink(backing.c_str());
        fail("fssconfig failed to create " + unit);
    }

    auto meta = loadMeta();
    Entry e;
    e.name = name;
    e.unit = unit;
    e.mountpoint = mp;
    e.backing = backing;
    e.created = static_cast<long long>(std::time(nullptr));
    meta.push_back(e);
    saveMeta(meta);

    std::printf("ok %s\n", unit.c_str());
}

void cmdDestroy(const std::string &name, bool removeBacking) {
    if (!validName(name)) fail("invalid snapshot name");

    auto meta = loadMeta();
    auto it = std::find_if(meta.begin(), meta.end(),
                           [&](const Entry &e) { return e.name == name; });
    if (it == meta.end()) fail("no such snapshot: " + name);

    Entry e = *it;

    if (!e.rodir.empty()) {
        if (runCmd({kUmount, e.rodir}, nullptr) != 0) fail("cannot unmount " + e.rodir);
    }
    if (activeUnits().count(e.unit) != 0) {
        if (runCmd({kFssconfig, "-u", e.unit}, nullptr) != 0) fail("fssconfig -u failed");
    }

    meta.erase(std::find_if(meta.begin(), meta.end(),
                            [&](const Entry &x) { return x.name == name; }));
    saveMeta(meta);

    if (removeBacking) {
        const std::string prefix = std::string(kBackingDir) + "/";
        if (e.backing.compare(0, prefix.size(), prefix) != 0)
            fail("refusing to remove a backing file outside " + std::string(kBackingDir));
        unlink(e.backing.c_str());
    }
    std::printf("ok\n");
}

void cmdMountRo(const std::string &name, const std::string &dirArg) {
    if (!validName(name)) fail("invalid snapshot name");
    if (!validAbsPath(dirArg)) fail("invalid directory");

    const std::string dir = realPath(dirArg);
    struct stat sb {};
    if (stat(dir.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) fail("not a directory: " + dir);
    if (!isEmptyDir(dir)) fail("mount directory must be empty: " + dir);

    auto meta = loadMeta();
    auto it = std::find_if(meta.begin(), meta.end(),
                           [&](const Entry &e) { return e.name == name; });
    if (it == meta.end()) fail("no such snapshot: " + name);
    if (!it->rodir.empty()) fail("already mounted at " + it->rodir);
    if (activeUnits().count(it->unit) == 0) fail("snapshot is not active: " + name);

    const std::string dev = "/dev/" + it->unit;
    if (runCmd({kMount, "-t", "ffs", "-r", dev, dir}, nullptr) != 0) fail("mount failed");

    it->rodir = dir;
    saveMeta(meta);
    std::printf("ok\n");
}

void cmdUmount(const std::string &dirArg) {
    if (!validAbsPath(dirArg)) fail("invalid directory");
    const std::string dir = realPath(dirArg);

    auto meta = loadMeta();
    auto it = std::find_if(meta.begin(), meta.end(),
                           [&](const Entry &e) { return e.rodir == dir; });
    if (it == meta.end()) fail("not an OctoSnap mount: " + dir);

    if (runCmd({kUmount, dir}, nullptr) != 0) fail("umount failed");

    it->rodir.clear();
    saveMeta(meta);
    std::printf("ok\n");
}

// ---- restore ----------------------------------------------------------------

#if defined(__APPLE__)
#define OS_ATIM(sb) (sb).st_atimespec
#define OS_MTIM(sb) (sb).st_mtimespec
#else
#define OS_ATIM(sb) (sb).st_atim
#define OS_MTIM(sb) (sb).st_mtim
#endif

const int kDirFlags = O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;

struct RestoreCtx {
    dev_t srcDev = 0;    // snapshot (read-only mount)
    dev_t dstDev = 0;    // live filesystem
    long files = 0, dirs = 0, links = 0, skipped = 0, errors = 0;
};

void restoreError(RestoreCtx &ctx, const std::string &rel, const std::string &what) {
    std::fprintf(stderr, "octosnap-helper: %s: %s\n", rel.c_str(), what.c_str());
    ++ctx.errors;
}

void restoreSkip(RestoreCtx &ctx, const std::string &rel, const std::string &why) {
    std::printf("skipped\t%s\t%s\n", rel.c_str(), why.c_str());
    ++ctx.skipped;
}

std::string errnoText() { return std::strerror(errno); }

// Relative path with no empty, "." or ".." components.
bool splitRelPath(const std::string &p, std::vector<std::string> &parts) {
    parts.clear();
    if (p.empty() || p[0] == '/' || p.size() >= PATH_MAX) return false;
    size_t start = 0;
    for (;;) {
        const size_t slash = p.find('/', start);
        const std::string c = p.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (c.empty() || c == "." || c == ".." || c.size() > NAME_MAX) return false;
        for (char ch : c)
            if (ch == '\t' || ch == '\n') return false;
        parts.push_back(c);
        if (slash == std::string::npos) break;
        start = slash + 1;
    }
    return true;
}

std::string tempName() {
    static unsigned counter = 0;
    return ".octosnap-restore." + std::to_string(getpid()) + "." + std::to_string(counter++);
}

// Owner first: chown clears set-id bits, so the mode must come after it.
bool setMetaFd(int fd, const struct stat &sb) {
    const struct timespec ts[2] = {OS_ATIM(sb), OS_MTIM(sb)};
    return fchown(fd, sb.st_uid, sb.st_gid) == 0 &&
           fchmod(fd, sb.st_mode & 07777) == 0 &&
           futimens(fd, ts) == 0;
}

// A name in dir that does not exist yet: <name>.octosnap-YYYYmmdd-HHMMSS[-N].
std::string backupName(int dir, const std::string &name) {
    char stamp[32];
    const std::time_t now = std::time(nullptr);
    std::strftime(stamp, sizeof stamp, "%Y%m%d-%H%M%S", std::localtime(&now));
    const std::string base = name + ".octosnap-" + stamp;

    struct stat sb {};
    for (int i = 0; i < 1000; ++i) {
        const std::string cand = i == 0 ? base : base + "-" + std::to_string(i);
        if (cand.size() > NAME_MAX) return std::string();
        if (fstatat(dir, cand.c_str(), &sb, AT_SYMLINK_NOFOLLOW) != 0 && errno == ENOENT) return cand;
    }
    return std::string();
}

// Refuses to replace an existing directory with a file or symlink.
bool dstAcceptsNonDir(RestoreCtx &ctx, int dstDir, const std::string &name, const std::string &rel) {
    struct stat dsb {};
    if (fstatat(dstDir, name.c_str(), &dsb, AT_SYMLINK_NOFOLLOW) == 0 && S_ISDIR(dsb.st_mode)) {
        restoreError(ctx, rel, "a directory exists at this path; restore with the current version kept");
        return false;
    }
    return true;
}

void restoreFile(RestoreCtx &ctx, int srcDir, int dstDir, const std::string &name,
                 const struct stat &sb, const std::string &rel) {
    if (!dstAcceptsNonDir(ctx, dstDir, name, rel)) return;

    const int in = openat(srcDir, name.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (in < 0) return restoreError(ctx, rel, "cannot open in snapshot: " + errnoText());

    struct stat isb {};
    if (fstat(in, &isb) != 0 || !S_ISREG(isb.st_mode) || isb.st_ino != sb.st_ino) {
        close(in);
        return restoreError(ctx, rel, "changed while restoring");
    }

    const std::string tmp = tempName();
    const int out = openat(dstDir, tmp.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (out < 0) {
        close(in);
        return restoreError(ctx, rel, "cannot create temporary file: " + errnoText());
    }

    bool ok = true;
    char buf[65536];
    for (;;) {
        const ssize_t n = read(in, buf, sizeof buf);
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            ok = false;
            break;
        }
        for (ssize_t off = 0; off < n;) {
            const ssize_t w = write(out, buf + off, static_cast<size_t>(n - off));
            if (w < 0) {
                if (errno == EINTR) continue;
                ok = false;
                break;
            }
            off += w;
        }
        if (!ok) break;
    }
    ok = ok && setMetaFd(out, isb) && fsync(out) == 0;
    const std::string err = errnoText();
    close(out);
    close(in);

    if (!ok || renameat(dstDir, tmp.c_str(), dstDir, name.c_str()) != 0) {
        const std::string why = ok ? errnoText() : err;
        unlinkat(dstDir, tmp.c_str(), 0);
        return restoreError(ctx, rel, "cannot write file: " + why);
    }
    ++ctx.files;
}

void restoreSymlink(RestoreCtx &ctx, int srcDir, int dstDir, const std::string &name,
                    const struct stat &sb, const std::string &rel) {
    if (!dstAcceptsNonDir(ctx, dstDir, name, rel)) return;

    char target[PATH_MAX];
    const ssize_t n = readlinkat(srcDir, name.c_str(), target, sizeof target - 1);
    if (n < 0) return restoreError(ctx, rel, "cannot read symlink: " + errnoText());
    target[n] = '\0';

    // The link is recreated as-is; it is never followed.
    const std::string tmp = tempName();
    if (symlinkat(target, dstDir, tmp.c_str()) != 0)
        return restoreError(ctx, rel, "cannot create symlink: " + errnoText());

    const struct timespec ts[2] = {OS_ATIM(sb), OS_MTIM(sb)};
    if (fchownat(dstDir, tmp.c_str(), sb.st_uid, sb.st_gid, AT_SYMLINK_NOFOLLOW) != 0 ||
        utimensat(dstDir, tmp.c_str(), ts, AT_SYMLINK_NOFOLLOW) != 0 ||
        renameat(dstDir, tmp.c_str(), dstDir, name.c_str()) != 0) {
        const std::string err = errnoText();
        unlinkat(dstDir, tmp.c_str(), 0);
        return restoreError(ctx, rel, "cannot place symlink: " + err);
    }
    ++ctx.links;
}

void restoreEntry(RestoreCtx &ctx, int srcDir, int dstDir, const std::string &name, const std::string &rel);

// An existing directory is merged: entries from the snapshot replace their
// live counterparts, entries that exist only on the live side are kept.
void restoreDir(RestoreCtx &ctx, int srcDir, int dstDir, const std::string &name,
                const struct stat &sb, const std::string &rel) {
    struct stat dsb {};
    if (fstatat(dstDir, name.c_str(), &dsb, AT_SYMLINK_NOFOLLOW) == 0) {
        if (!S_ISDIR(dsb.st_mode))
            return restoreError(ctx, rel, "not a directory on the live filesystem; "
                                          "restore with the current version kept");
    } else if (errno != ENOENT || mkdirat(dstDir, name.c_str(), 0700) != 0) {
        return restoreError(ctx, rel, "cannot create directory: " + errnoText());
    }

    const int dfd = openat(dstDir, name.c_str(), kDirFlags);
    if (dfd < 0) return restoreError(ctx, rel, "cannot open directory: " + errnoText());
    if (fstat(dfd, &dsb) != 0 || dsb.st_dev != ctx.dstDev) {
        close(dfd);
        return restoreSkip(ctx, rel, "another filesystem is mounted here");
    }

    const int sfd = openat(srcDir, name.c_str(), kDirFlags);
    const int listFd = sfd < 0 ? -1 : dup(sfd);
    DIR *dp = listFd < 0 ? nullptr : fdopendir(listFd);
    if (!dp) {
        if (listFd >= 0) close(listFd);
        if (sfd >= 0) close(sfd);
        close(dfd);
        return restoreError(ctx, rel, "cannot read directory in snapshot: " + errnoText());
    }

    std::vector<std::string> children;
    while (struct dirent *de = readdir(dp)) {
        if (std::strcmp(de->d_name, ".") != 0 && std::strcmp(de->d_name, "..") != 0)
            children.emplace_back(de->d_name);
    }
    closedir(dp);

    for (const auto &child : children) restoreEntry(ctx, sfd, dfd, child, rel + "/" + child);

    // After the children, so their creation does not bump the restored mtime.
    if (!setMetaFd(dfd, sb)) restoreError(ctx, rel, "cannot set directory attributes: " + errnoText());
    close(sfd);
    close(dfd);
    ++ctx.dirs;
}

void restoreEntry(RestoreCtx &ctx, int srcDir, int dstDir, const std::string &name, const std::string &rel) {
    struct stat sb {};
    if (fstatat(srcDir, name.c_str(), &sb, AT_SYMLINK_NOFOLLOW) != 0)
        return restoreError(ctx, rel, "not found in snapshot");
    if (sb.st_dev != ctx.srcDev) return restoreSkip(ctx, rel, "another filesystem is mounted here");

    if (S_ISREG(sb.st_mode))       restoreFile(ctx, srcDir, dstDir, name, sb, rel);
    else if (S_ISLNK(sb.st_mode))  restoreSymlink(ctx, srcDir, dstDir, name, sb, rel);
    else if (S_ISDIR(sb.st_mode))  restoreDir(ctx, srcDir, dstDir, name, sb, rel);
    else                           restoreSkip(ctx, rel, "special file");
}

// Opens the parent directories of parts.back() on both sides. Parents that
// no longer exist on the live filesystem are recreated from the snapshot.
bool openParents(RestoreCtx &ctx, int srcRoot, int dstRoot, const std::vector<std::string> &parts,
                 int &sp, int &dp) {
    sp = dup(srcRoot);
    dp = dup(dstRoot);
    std::string rel;
    for (size_t i = 0; i + 1 < parts.size(); ++i) {
        const std::string &c = parts[i];
        rel += (rel.empty() ? "" : "/") + c;

        struct stat sb {}, dsb {};
        const int nsp = openat(sp, c.c_str(), kDirFlags);
        if (nsp < 0 || fstat(nsp, &sb) != 0 || sb.st_dev != ctx.srcDev) {
            if (nsp >= 0) close(nsp);
            restoreError(ctx, rel, "not a directory in the snapshot");
            return false;
        }

        int ndp = openat(dp, c.c_str(), kDirFlags);
        if (ndp < 0 && errno == ENOENT && mkdirat(dp, c.c_str(), 0700) == 0) {
            ndp = openat(dp, c.c_str(), kDirFlags);
            if (ndp >= 0 && setMetaFd(ndp, sb)) std::printf("created\t%s\n", rel.c_str());
        }
        if (ndp < 0 || fstat(ndp, &dsb) != 0 || dsb.st_dev != ctx.dstDev) {
            if (ndp >= 0) close(ndp);
            close(nsp);
            restoreError(ctx, rel, "not a directory on the live filesystem, or a symlink");
            return false;
        }

        close(sp);
        close(dp);
        sp = nsp;
        dp = ndp;
    }
    return true;
}

// Restores one top-level item. Returns false if anything under it failed.
bool restoreTop(RestoreCtx &ctx, int srcRoot, int dstRoot, const std::string &rel, bool renameExisting) {
    std::vector<std::string> parts;
    splitRelPath(rel, parts);
    const long errorsBefore = ctx.errors;

    int sp = -1, dp = -1;
    if (openParents(ctx, srcRoot, dstRoot, parts, sp, dp)) {
        const std::string &name = parts.back();
        struct stat sb {}, dsb {};

        if (fstatat(sp, name.c_str(), &sb, AT_SYMLINK_NOFOLLOW) != 0) {
            restoreError(ctx, rel, "not found in snapshot");
        } else if (renameExisting && fstatat(dp, name.c_str(), &dsb, AT_SYMLINK_NOFOLLOW) == 0) {
            const std::string backup = backupName(dp, name);
            if (backup.empty() || renameat(dp, name.c_str(), dp, backup.c_str()) != 0) {
                restoreError(ctx, rel, "cannot rename the current version");
            } else {
                const size_t slash = rel.rfind('/');
                const std::string dir = slash == std::string::npos ? "" : rel.substr(0, slash + 1);
                std::printf("backup\t%s\t%s%s\n", rel.c_str(), dir.c_str(), backup.c_str());
                restoreEntry(ctx, sp, dp, name, rel);
            }
        } else {
            restoreEntry(ctx, sp, dp, name, rel);
        }
    }
    if (sp >= 0) close(sp);
    if (dp >= 0) close(dp);

    const bool ok = ctx.errors == errorsBefore;
    if (ok) std::printf("restored\t%s\n", rel.c_str());
    return ok;
}

void cmdRestore(const std::string &name, bool renameExisting, const std::vector<std::string> &rels) {
    if (!validName(name)) fail("invalid snapshot name");
    std::vector<std::string> parts;
    for (const auto &r : rels)
        if (!splitRelPath(r, parts)) fail("invalid relative path: " + r);

    const auto meta = loadMeta();
    auto it = std::find_if(meta.begin(), meta.end(),
                           [&](const Entry &e) { return e.name == name; });
    if (it == meta.end()) fail("no such snapshot: " + name);
    if (activeUnits().count(it->unit) == 0) fail("snapshot is not active: " + name);
    if (it->rodir.empty()) fail("snapshot is not mounted read-only: " + name);
    if (!isMountedFrom(it->rodir, "/dev/" + it->unit))
        fail(it->rodir + " is not mounted from /dev/" + it->unit);
    if (!isFfsMountPoint(it->mountpoint)) fail("not a mounted FFS filesystem: " + it->mountpoint);

    const int srcRoot = open(it->rodir.c_str(), kDirFlags);
    const int dstRoot = open(it->mountpoint.c_str(), kDirFlags);
    struct stat ssb {}, dsb {};
    if (srcRoot < 0 || dstRoot < 0 || fstat(srcRoot, &ssb) != 0 || fstat(dstRoot, &dsb) != 0)
        fail("cannot open snapshot or live filesystem");
    if (ssb.st_dev == dsb.st_dev) fail("snapshot and live filesystem are the same device");

    RestoreCtx ctx;
    ctx.srcDev = ssb.st_dev;
    ctx.dstDev = dsb.st_dev;
    for (const auto &r : rels) restoreTop(ctx, srcRoot, dstRoot, r, renameExisting);
    close(srcRoot);
    close(dstRoot);

    std::printf("summary\t%ld\t%ld\t%ld\t%ld\t%ld\n",
                ctx.files, ctx.dirs, ctx.links, ctx.skipped, ctx.errors);
    if (ctx.errors > 0) fail(std::to_string(ctx.errors) + " error(s) during restore");
    std::printf("ok\n");
}

} // namespace

int main(int argc, char **argv) {
    if (geteuid() != 0) fail("must run as root (via doas or sudo)", 2);

    umask(077);
    setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);

    if (argc < 2) usage();
    const std::string cmd = argv[1];

    if (cmd == "list" && argc == 2) {
        cmdList();
    } else if (cmd == "create" && argc == 4) {
        cmdCreate(argv[2], argv[3]);
    } else if (cmd == "destroy" && (argc == 3 || (argc == 4 && std::string(argv[3]) == "--remove-backing"))) {
        cmdDestroy(argv[2], argc == 4);
    } else if (cmd == "mount-ro" && argc == 4) {
        cmdMountRo(argv[2], argv[3]);
    } else if (cmd == "umount" && argc == 3) {
        cmdUmount(argv[2]);
    } else if (cmd == "restore" && argc >= 4) {
        const bool renameExisting = std::string(argv[3]) == "--rename-existing";
        const int first = renameExisting ? 4 : 3;
        if (first >= argc) usage();
        cmdRestore(argv[2], renameExisting, std::vector<std::string>(argv + first, argv + argc));
    } else {
        usage();
    }
    return 0;
}
