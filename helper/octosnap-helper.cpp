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

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
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
         "mount-ro <name> <dir> | umount <dir>", 2);
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
    struct statvfs *mnts = nullptr;
    const int n = getmntinfo(&mnts, ST_NOWAIT);
    for (int i = 0; i < n; ++i)
        if (mp == mnts[i].f_mntonname && std::strcmp(mnts[i].f_fstypename, "ffs") == 0)
            return true;
    return false;
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
    } else {
        usage();
    }
    return 0;
}
