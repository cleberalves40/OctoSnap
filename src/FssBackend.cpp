#include "FssBackend.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSettings>

namespace {
const char *kHelperDefault = "/usr/pkg/libexec/octosnap-helper";
}

FssBackend::FssBackend(QObject *parent) : QObject(parent) {}

HelperResult FssBackend::runHelper(const QStringList &args) const {
    QSettings s;
    const QString tool    = s.value(QStringLiteral("elevator/tool"), QStringLiteral("doas")).toString();
    const QString askpass = s.value(QStringLiteral("elevator/askpass")).toString();
    const QString helper  = s.value(QStringLiteral("elevator/helper"), QLatin1String(kHelperDefault)).toString();

    HelperResult r;
    QProcess p;
    QStringList argv;

    if (tool == QLatin1String("sudo")) {
        argv << QStringLiteral("-A") << QStringLiteral("--") << helper << args;
        if (!askpass.isEmpty()) {
            auto env = QProcessEnvironment::systemEnvironment();
            env.insert(QStringLiteral("SUDO_ASKPASS"), askpass);
            p.setProcessEnvironment(env);
        }
        p.start(QStringLiteral("sudo"), argv);
    } else {
        argv << helper << args;
        p.start(QStringLiteral("doas"), argv);
    }

    if (!p.waitForStarted(5000)) {
        r.error = tr("Could not start %1").arg(tool);
        return r;
    }

    p.waitForFinished(-1);   // may block while the user types a password

    r.output = QString::fromUtf8(p.readAllStandardOutput());
    const QString err = QString::fromUtf8(p.readAllStandardError()).trimmed();

    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        r.error = err.isEmpty() ? tr("Helper failed with code %1").arg(p.exitCode()) : err;
        return r;
    }
    r.ok = true;
    return r;
}

QVector<FileSystemInfo> FssBackend::filesystems() const {
    QVector<FileSystemInfo> out;

    QProcess p;
    p.start(QStringLiteral("/sbin/mount"), {QStringLiteral("-p")});
    if (!p.waitForFinished(5000)) return out;

    static const QRegularExpression ws(QStringLiteral("\\s+"));
    const auto lines = QString::fromUtf8(p.readAllStandardOutput())
                           .split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList f = line.split(ws, Qt::SkipEmptyParts);
        if (f.size() >= 3 && f[2] == QLatin1String("ffs"))
            out.append(FileSystemInfo{f[0], f[1]});
    }
    return out;
}

HelperResult FssBackend::snapshots(QVector<SnapshotInfo> &out) const {
    HelperResult r = runHelper({QStringLiteral("list")});
    if (!r.ok) return r;

    out.clear();
    const auto lines = r.output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList f = line.split(QLatin1Char('\t'));
        if (f.size() < 8) continue;

        SnapshotInfo s;
        s.name = f[0];
        s.unit = f[1];
        s.mountPoint = f[2];
        s.backingFile = f[3];
        s.created = QDateTime::fromSecsSinceEpoch(f[4].toLongLong());
        s.readOnlyPath = f[5];
        s.active = (f[6] == QLatin1String("1"));
        s.sizeBytes = f[7].toLongLong();
        out.append(s);
    }
    return r;
}

HelperResult FssBackend::createSnapshot(const QString &name, const QString &mountPoint) const {
    return runHelper({QStringLiteral("create"), name, mountPoint});
}

HelperResult FssBackend::destroySnapshot(const QString &name, bool removeBacking) const {
    QStringList args{QStringLiteral("destroy"), name};
    if (removeBacking) args << QStringLiteral("--remove-backing");
    return runHelper(args);
}

HelperResult FssBackend::mountSnapshot(const QString &name, const QString &dir) const {
    return runHelper({QStringLiteral("mount-ro"), name, dir});
}

HelperResult FssBackend::unmountSnapshot(const QString &dir) const {
    return runHelper({QStringLiteral("umount"), dir});
}
