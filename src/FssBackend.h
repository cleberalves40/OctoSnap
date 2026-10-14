#pragma once

#include <QDateTime>
#include <QObject>
#include <QStringList>
#include <QVector>

struct FileSystemInfo {
    QString device;
    QString mountPoint;
};

struct SnapshotInfo {
    QString name;
    QString unit;            // fss0 .. fssN
    QString mountPoint;      // filesystem being snapshotted
    QString backingFile;
    QDateTime created;
    QString readOnlyPath;    // empty if not mounted by OctoSnap
    qint64 sizeBytes = 0;    // allocated size of the backing store
    bool active = false;     // false = recorded but no longer in fssconfig -l
};

struct HelperResult {
    bool ok = false;
    QString error;
    QString output;
};

class FssBackend : public QObject {
    Q_OBJECT

public:
    explicit FssBackend(QObject *parent = nullptr);

    // Unprivileged: reads the mount table.
    QVector<FileSystemInfo> filesystems() const;

    // Privileged operations, routed through the helper.
    HelperResult snapshots(QVector<SnapshotInfo> &out) const;
    HelperResult createSnapshot(const QString &name, const QString &mountPoint) const;
    HelperResult destroySnapshot(const QString &name, bool removeBacking) const;
    HelperResult mountSnapshot(const QString &name, const QString &dir) const;
    HelperResult unmountSnapshot(const QString &dir) const;

private:
    HelperResult runHelper(const QStringList &args) const;
};
