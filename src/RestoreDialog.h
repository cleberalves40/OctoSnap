#pragma once

#include <QDialog>

#include "FssBackend.h"

class QCheckBox;
class QFileSystemModel;
class QPushButton;
class QTreeView;
class QTreeWidget;

// Lets the user pick files and directories inside a snapshot's read-only
// mount and previews what restoring them to the live filesystem will do.
class RestoreDialog : public QDialog {
    Q_OBJECT

public:
    explicit RestoreDialog(const SnapshotInfo &snapshot, QWidget *parent = nullptr);

    // Selected items, relative to the filesystem root, without nested duplicates.
    QStringList relativePaths() const { return m_relPaths; }
    bool keepCurrent() const;

private slots:
    void updatePreview();

private:
    SnapshotInfo m_snapshot;
    QStringList m_relPaths;

    QFileSystemModel *m_model = nullptr;
    QTreeView *m_tree = nullptr;
    QTreeWidget *m_preview = nullptr;
    QCheckBox *m_keepCurrent = nullptr;
    QPushButton *m_restoreButton = nullptr;
};
