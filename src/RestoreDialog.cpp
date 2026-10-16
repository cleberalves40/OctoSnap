#include "RestoreDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSplitter>
#include <QTreeView>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

enum class Kind { Missing, File, Dir, Link };

Kind kindOf(const QFileInfo &fi) {
    if (fi.isSymLink()) return Kind::Link;   // also true for dangling links
    if (!fi.exists()) return Kind::Missing;
    return fi.isDir() ? Kind::Dir : Kind::File;
}

QString describe(const QFileInfo &fi) {
    const QString when = fi.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    switch (kindOf(fi)) {
    case Kind::Missing: return RestoreDialog::tr("missing");
    case Kind::Link:    return RestoreDialog::tr("symlink → %1").arg(fi.symLinkTarget());
    case Kind::Dir:     return RestoreDialog::tr("directory, %1").arg(when);
    case Kind::File:    return QStringLiteral("%1, %2").arg(QLocale().formattedDataSize(fi.size()), when);
    }
    return QString();
}

} // namespace

RestoreDialog::RestoreDialog(const SnapshotInfo &snapshot, QWidget *parent)
    : QDialog(parent), m_snapshot(snapshot) {
    setWindowTitle(tr("Restore from snapshot \"%1\"").arg(snapshot.name));
    resize(900, 650);

    auto *intro = new QLabel(tr(
        "Select the files and directories to restore. They are copied from the snapshot "
        "(mounted at %1) back to the same place on %2.")
        .arg(snapshot.readOnlyPath, snapshot.mountPoint), this);
    intro->setWordWrap(true);

    auto *splitter = new QSplitter(Qt::Vertical, this);

    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    m_model->setRootPath(snapshot.readOnlyPath);

    m_tree = new QTreeView(splitter);
    m_tree->setModel(m_model);
    m_tree->setRootIndex(m_model->index(snapshot.readOnlyPath));
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->setSortingEnabled(true);
    m_tree->sortByColumn(0, Qt::AscendingOrder);
    m_tree->header()->resizeSection(0, 320);
    connect(m_tree->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RestoreDialog::updatePreview);

    m_preview = new QTreeWidget(splitter);
    m_preview->setRootIsDecorated(false);
    m_preview->setHeaderLabels({tr("Item"), tr("In snapshot"), tr("Current version"), tr("Action")});
    m_preview->header()->resizeSection(0, 260);
    m_preview->header()->resizeSection(1, 200);
    m_preview->header()->resizeSection(2, 200);

    splitter->addWidget(m_tree);
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    m_keepCurrent = new QCheckBox(tr("Keep the current version of each item as a backup "
                                     "(renamed to <item>.octosnap-<date>)"), this);
    m_keepCurrent->setChecked(true);
    connect(m_keepCurrent, &QCheckBox::toggled, this, &RestoreDialog::updatePreview);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_restoreButton = buttons->addButton(tr("Restore"), QDialogButtonBox::AcceptRole);
    m_restoreButton->setIcon(QIcon::fromTheme(QStringLiteral("document-revert")));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(splitter, 1);
    layout->addWidget(m_keepCurrent);
    layout->addWidget(buttons);

    updatePreview();
}

bool RestoreDialog::keepCurrent() const {
    return m_keepCurrent->isChecked();
}

void RestoreDialog::updatePreview() {
    const QDir snapRoot(m_snapshot.readOnlyPath);
    const QDir liveRoot(m_snapshot.mountPoint);

    QStringList rels;
    for (const QModelIndex &idx : m_tree->selectionModel()->selectedRows(0)) {
        const QString rel = snapRoot.relativeFilePath(m_model->filePath(idx));
        if (!rel.isEmpty() && rel != QLatin1String(".") && !rel.startsWith(QLatin1String("..")))
            rels << rel;
    }

    // Selecting a directory already covers everything below it.
    rels.sort();
    m_relPaths.clear();
    for (const QString &rel : rels) {
        if (!m_relPaths.isEmpty() && rel.startsWith(m_relPaths.last() + QLatin1Char('/'))) continue;
        m_relPaths << rel;
    }

    m_preview->clear();
    bool conflict = false;
    for (const QString &rel : m_relPaths) {
        const QFileInfo snap(snapRoot.filePath(rel));
        const QFileInfo live(liveRoot.filePath(rel));
        const Kind sk = kindOf(snap);
        const Kind lk = kindOf(live);

        QString action;
        bool rowConflict = false;
        if (lk == Kind::Missing) {
            action = tr("Recreate");
        } else if (keepCurrent()) {
            action = tr("Restore, keep current as backup");
        } else if (sk == Kind::Dir && lk == Kind::Dir) {
            action = tr("Merge into current directory");
        } else if ((sk == Kind::Dir) != (lk == Kind::Dir)) {
            action = tr("Conflict: file and directory");
            rowConflict = true;
        } else if (sk == Kind::File && lk == Kind::File && snap.size() == live.size() &&
                   snap.lastModified() == live.lastModified()) {
            action = tr("Overwrite (looks unchanged)");
        } else {
            action = tr("Overwrite");
        }

        auto *item = new QTreeWidgetItem(m_preview, {rel, describe(snap), describe(live), action});
        if (rowConflict)
            item->setToolTip(3, tr("Enable \"keep the current version\" to restore this item."));
        conflict = conflict || rowConflict;
    }

    m_restoreButton->setEnabled(!m_relPaths.isEmpty() && !conflict);
}
