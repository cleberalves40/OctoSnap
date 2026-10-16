#include "MainWindow.h"
#include "RestoreDialog.h"
#include "Theme.h"

#include <QAbstractItemView>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>

namespace {

// Same style as bectl: "67.0M", "2.04G", "364M".
QString humanSize(qint64 bytes) {
    if (bytes <= 0) return QStringLiteral("-");
    static const char *units[] = {"B", "K", "M", "G", "T"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 4) {
        v /= 1024.0;
        ++u;
    }
    const int prec = (u == 0) ? 0 : (v < 10.0 ? 2 : (v < 100.0 ? 1 : 0));
    return QString::number(v, 'f', prec) + QLatin1String(units[u]);
}

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("OctoSnap — FFS Snapshots"));
    resize(1100, 700);

    buildUi();
    buildToolBar();
    buildMenus();
    updateThemeLabel();
    refreshAll();
    log(tr("OctoSnap started."));
}

void MainWindow::buildUi() {
    // Table on top, Information / Output / Help tabs below, as in bectl.
    auto *vertical = new QSplitter(Qt::Vertical, this);

    m_snapTable = new QTableWidget(0, 5, vertical);
    m_snapTable->setHorizontalHeaderLabels({
        tr("Snapshot"), tr("Active"), tr("Mountpoint"), tr("Space"), tr("Created")});
    m_snapTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_snapTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_snapTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_snapTable->setAlternatingRowColors(true);
    m_snapTable->verticalHeader()->setVisible(false);
    m_snapTable->horizontalHeader()->setStretchLastSection(true);
    m_snapTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    connect(m_snapTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onTableSelectionChanged);

    m_tabs = new QTabWidget(vertical);

    m_info = new QPlainTextEdit(m_tabs);
    m_info->setReadOnly(true);

    m_output = new QPlainTextEdit(m_tabs);
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(2000);

    m_help = new QPlainTextEdit(m_tabs);
    m_help->setReadOnly(true);
    m_help->setPlainText(tr(
        "Snapshots use fss(4) with a copy-on-write backing store.\n\n"
        "New snapshot: choose a mounted filesystem, then give the snapshot a name.\n"
        "Mount read-only: mounts an active snapshot in an empty directory you choose.\n"
        "Unmount: releases a read-only mount made by OctoSnap.\n"
        "Restore: pick files or directories inside the snapshot and copy them back to\n"
        "the live filesystem. A preview shows what changes; by default the current\n"
        "version of each item is kept as <item>.octosnap-<date>. Directories are merged:\n"
        "files created after the snapshot are kept.\n"
        "Destroy selected: removes the snapshot. Destroying data cannot be undone.\n\n"
        "Snapshots do not survive a reboot. Those marked Stale must be destroyed."));

    m_tabs->addTab(m_info, tr("Information"));
    m_tabs->addTab(m_output, tr("Output"));
    m_tabs->addTab(m_help, tr("Help"));

    vertical->addWidget(m_snapTable);
    vertical->addWidget(m_tabs);
    vertical->setStretchFactor(0, 3);
    vertical->setStretchFactor(1, 1);

    setCentralWidget(vertical);

    m_themeLabel = new QLabel(this);
    m_countLabel = new QLabel(this);
    statusBar()->addWidget(m_themeLabel);
    statusBar()->addPermanentWidget(m_countLabel);
}

void MainWindow::buildToolBar() {
    auto *tb = addToolBar(tr("Actions"));
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_refreshAct = tb->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), tr("Refresh"));
    m_refreshAct->setShortcut(QKeySequence::Refresh);
    connect(m_refreshAct, &QAction::triggered, this, &MainWindow::refreshAll);

    tb->addSeparator();

    m_createAct = tb->addAction(QIcon::fromTheme(QStringLiteral("document-new")), tr("Create"));
    m_createAct->setShortcut(QKeySequence::New);
    connect(m_createAct, &QAction::triggered, this, &MainWindow::onCreate);

    tb->addSeparator();

    m_mountAct = tb->addAction(QIcon::fromTheme(QStringLiteral("drive-harddisk")),
                               tr("Mount read-only"));
    m_mountAct->setEnabled(false);
    connect(m_mountAct, &QAction::triggered, this, &MainWindow::onMount);

    m_unmountAct = tb->addAction(QIcon::fromTheme(QStringLiteral("media-eject")), tr("Unmount"));
    m_unmountAct->setEnabled(false);
    connect(m_unmountAct, &QAction::triggered, this, &MainWindow::onUnmount);

    m_restoreAct = tb->addAction(QIcon::fromTheme(QStringLiteral("document-revert")), tr("Restore"));
    m_restoreAct->setEnabled(false);
    m_restoreAct->setToolTip(tr("Copy files or directories from the snapshot back to the live filesystem"));
    connect(m_restoreAct, &QAction::triggered, this, &MainWindow::onRestore);

    tb->addSeparator();

    m_destroyAct = tb->addAction(QIcon::fromTheme(QStringLiteral("edit-delete")),
                                 tr("Destroy selected"));
    m_destroyAct->setEnabled(false);
    connect(m_destroyAct, &QAction::triggered, this, &MainWindow::onDestroy);

    // Spacer pushes the filter to the right, as in bectl.
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer);

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("Filter by name…"));
    m_filter->setClearButtonEnabled(true);
    m_filter->setMinimumWidth(220);
    connect(m_filter, &QLineEdit::textChanged, this, [this]() { populateSnapshotTable(); });
    tb->addWidget(m_filter);
}

void MainWindow::buildMenus() {
    auto *file = menuBar()->addMenu(tr("&File"));
    file->addAction(m_refreshAct);
    file->addSeparator();
    file->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);

    auto *actions = menuBar()->addMenu(tr("&Actions"));
    actions->addAction(m_createAct);
    actions->addAction(m_destroyAct);
    actions->addSeparator();
    actions->addAction(m_mountAct);
    actions->addAction(m_unmountAct);
    actions->addSeparator();
    actions->addAction(m_restoreAct);

    auto *themeMenu = menuBar()->addMenu(tr("&Theme"));
    auto *themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);
    const ThemeId currentTheme = Theme::current();
    for (ThemeId id : Theme::all()) {
        QAction *a = themeMenu->addAction(Theme::name(id));
        a->setCheckable(true);
        a->setChecked(id == currentTheme);
        themeGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, id]() {
            Theme::setCurrent(id);
            Theme::apply(id);
            updateThemeLabel();
            log(tr("Theme changed to %1.").arg(Theme::name(id)));
        });
    }

    auto *langMenu = menuBar()->addMenu(tr("&Language"));
    const QString currentLang = QSettings().value(QStringLiteral("ui/language"),
                                                  QStringLiteral("system")).toString();
    struct LangOption { QString code; QString label; };
    const QList<LangOption> langs = {
        {QStringLiteral("system"), tr("System default")},
        {QStringLiteral("en"),     QStringLiteral("English")},
        {QStringLiteral("pt_BR"),  QStringLiteral("Português (Brasil)")},
    };
    auto *langGroup = new QActionGroup(this);
    langGroup->setExclusive(true);
    for (const auto &opt : langs) {
        QAction *a = langMenu->addAction(opt.label);
        a->setCheckable(true);
        a->setChecked(opt.code == currentLang);
        langGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, code = opt.code]() {
            QSettings().setValue(QStringLiteral("ui/language"), code);
            QMessageBox::information(this, tr("Language"),
                                     tr("Restart OctoSnap to apply the language change."));
        });
    }

    auto *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About OctoSnap"), this, &MainWindow::onAbout);
}

void MainWindow::refreshAll() {
    // Remember the selected snapshot by name, so a refresh keeps the selection.
    const int previousIdx = selectedIndex();
    const QString previous = previousIdx >= 0 ? m_all[previousIdx].name : QString();

    const HelperResult r = m_backend.snapshots(m_all);
    if (!r.ok) {
        m_all.clear();
        log(tr("Could not read snapshots: %1").arg(r.error));
    }

    populateSnapshotTable();

    if (!previous.isEmpty()) {
        for (int row = 0; row < m_snapTable->rowCount(); ++row) {
            if (m_snapTable->item(row, 0)->text() == previous) {
                m_snapTable->setCurrentCell(row, 0);
                break;
            }
        }
    }
    log(tr("List updated (%n snapshot(s)).", nullptr, static_cast<int>(m_all.size())));
}

void MainWindow::populateSnapshotTable() {
    const QString needle = m_filter->text().trimmed();

    m_visible.clear();
    m_snapTable->setRowCount(0);

    for (int i = 0; i < m_all.size(); ++i) {
        const SnapshotInfo &s = m_all[i];
        if (!needle.isEmpty() && !s.name.contains(needle, Qt::CaseInsensitive)) continue;

        const int row = m_snapTable->rowCount();
        m_snapTable->insertRow(row);
        m_visible.append(i);

        QString active;
        if (!s.active)                      active = tr("Stale");
        else if (!s.readOnlyPath.isEmpty()) active = tr("RO");
        else                                active = QStringLiteral("-");

        m_snapTable->setItem(row, 0, new QTableWidgetItem(s.name));
        m_snapTable->setItem(row, 1, new QTableWidgetItem(active));
        m_snapTable->setItem(row, 2, new QTableWidgetItem(s.mountPoint));

        auto *space = new QTableWidgetItem(humanSize(s.sizeBytes));
        space->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_snapTable->setItem(row, 3, space);

        m_snapTable->setItem(row, 4, new QTableWidgetItem(
            s.created.toString(QStringLiteral("yyyy-MM-dd HH:mm"))));
    }

    updateStatusBar();
    onTableSelectionChanged();
}

int MainWindow::selectedIndex() const {
    const int row = m_snapTable->currentRow();
    if (row < 0 || row >= m_visible.size()) return -1;
    return m_visible[row];
}

void MainWindow::onTableSelectionChanged() {
    const int idx = selectedIndex();
    const bool has = idx >= 0;
    const bool mounted = has && !m_all[idx].readOnlyPath.isEmpty();

    m_destroyAct->setEnabled(has);
    m_mountAct->setEnabled(has && m_all[idx].active && !mounted);
    m_unmountAct->setEnabled(mounted);
    m_restoreAct->setEnabled(has && m_all[idx].active);

    updateInformation(idx);
}

void MainWindow::updateInformation(int index) {
    if (index < 0) {
        m_info->setPlainText(tr("Select a snapshot to see its details."));
        return;
    }
    const SnapshotInfo &s = m_all[index];

    QStringList lines;
    lines << tr("Snapshot: %1").arg(s.name)
          << tr("Unit: %1").arg(s.unit)
          << tr("Filesystem: %1").arg(s.mountPoint)
          << tr("Backing store: %1").arg(s.backingFile)
          << tr("Space used: %1 (%2 bytes)").arg(humanSize(s.sizeBytes)).arg(s.sizeBytes)
          << tr("Created: %1").arg(s.created.toString(Qt::ISODate))
          << tr("State: %1").arg(s.active ? tr("Active") : tr("Stale (not in fssconfig -l)"))
          << tr("Read-only mount: %1").arg(s.readOnlyPath.isEmpty() ? tr("none") : s.readOnlyPath);
    m_info->setPlainText(lines.join(QLatin1Char('\n')));
}

void MainWindow::updateStatusBar() {
    m_countLabel->setText(tr("%n snapshot(s)", nullptr, static_cast<int>(m_all.size())));
}

void MainWindow::updateThemeLabel() {
    m_themeLabel->setText(tr("Theme: %1").arg(Theme::name(Theme::current())));
}

void MainWindow::log(const QString &message) {
    m_output->appendPlainText(QStringLiteral("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), message));
}

void MainWindow::onCreate() {
    const QVector<FileSystemInfo> filesystems = m_backend.filesystems();
    if (filesystems.isEmpty()) {
        log(tr("No FFS filesystem is mounted."));
        return;
    }

    // Pick the filesystem to snapshot. With a single FFS mount, there is nothing to choose.
    QString mountPoint = filesystems.first().mountPoint;
    if (filesystems.size() > 1) {
        QStringList items;
        for (const auto &fs : filesystems) items << fs.mountPoint;

        bool ok = false;
        mountPoint = QInputDialog::getItem(
            this, tr("New snapshot"), tr("Filesystem:"), items, 0, false, &ok);
        if (!ok || mountPoint.isEmpty()) return;
    }

    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("New snapshot"),
        tr("Snapshot name (letters, digits, '.', '_', '-'):"),
        QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || name.isEmpty()) return;

    const HelperResult r = m_backend.createSnapshot(name, mountPoint);
    if (!r.ok) {
        reportError(tr("Create snapshot"), r.error);
        return;
    }
    log(tr("Snapshot '%1' created on %2.").arg(name, mountPoint));
    refreshAll();
}

void MainWindow::onDestroy() {
    const int idx = selectedIndex();
    if (idx < 0) return;
    const SnapshotInfo s = m_all[idx];

    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Destroy snapshot"));
    box.setText(tr("Destroy snapshot \"%1\"?").arg(s.name));
    box.setInformativeText(tr(
        "The live filesystem is not changed, but you lose the ability to "
        "recover data from this snapshot. This cannot be undone."));

    auto *removeBacking = new QCheckBox(tr("Also delete the backing store file"), &box);
    box.setCheckBox(removeBacking);

    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);
    if (box.exec() != QMessageBox::Yes) return;

    const HelperResult r = m_backend.destroySnapshot(s.name, removeBacking->isChecked());
    if (!r.ok) {
        reportError(tr("Destroy snapshot"), r.error);
        return;
    }
    log(tr("Snapshot '%1' destroyed.").arg(s.name));
    refreshAll();
}

bool MainWindow::mountInteractive(const QString &name) {
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Choose an empty directory for the read-only mount"),
        QDir::homePath());
    if (dir.isEmpty()) return false;

    const HelperResult r = m_backend.mountSnapshot(name, dir);
    if (!r.ok) {
        reportError(tr("Mount snapshot"), r.error);
        return false;
    }
    log(tr("Snapshot '%1' mounted read-only at %2.").arg(name, dir));
    refreshAll();
    return true;
}

void MainWindow::onMount() {
    const int idx = selectedIndex();
    if (idx < 0) return;
    mountInteractive(m_all[idx].name);
}

void MainWindow::onUnmount() {
    const int idx = selectedIndex();
    if (idx < 0) return;
    const QString dir = m_all[idx].readOnlyPath;
    if (dir.isEmpty()) return;

    const HelperResult r = m_backend.unmountSnapshot(dir);
    if (!r.ok) {
        reportError(tr("Unmount snapshot"), r.error);
        return;
    }
    log(tr("Unmounted %1.").arg(dir));
    refreshAll();
}

void MainWindow::onRestore() {
    int idx = selectedIndex();
    if (idx < 0 || !m_all[idx].active) return;
    const QString name = m_all[idx].name;

    // Restore reads from the read-only mount, so the snapshot must be mounted.
    if (m_all[idx].readOnlyPath.isEmpty()) {
        const auto answer = QMessageBox::question(
            this, tr("Restore"),
            tr("Snapshot \"%1\" must be mounted read-only before its files can be "
               "restored. Mount it now?").arg(name));
        if (answer != QMessageBox::Yes || !mountInteractive(name)) return;

        idx = selectedIndex();   // refreshAll() rebuilt m_all
        if (idx < 0 || m_all[idx].name != name || m_all[idx].readOnlyPath.isEmpty()) return;
    }
    const SnapshotInfo s = m_all[idx];

    RestoreDialog dialog(s, this);
    if (dialog.exec() != QDialog::Accepted) return;

    const QStringList items = dialog.relativePaths();
    const bool keepCurrent = dialog.keepCurrent();
    if (items.isEmpty()) return;

    if (!keepCurrent) {
        QMessageBox box(this);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(tr("Restore"));
        box.setText(tr("Overwrite %n item(s) on %1 with the version from \"%2\"?",
                       nullptr, static_cast<int>(items.size())).arg(s.mountPoint, s.name));
        box.setInformativeText(tr("The current contents of these items are replaced and "
                                  "cannot be recovered afterwards."));
        box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        box.setDefaultButton(QMessageBox::No);
        if (box.exec() != QMessageBox::Yes) return;
    }

    log(tr("Restoring %n item(s) from '%1'…", nullptr, static_cast<int>(items.size())).arg(s.name));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const HelperResult r = m_backend.restoreItems(s.name, items, keepCurrent);
    QApplication::restoreOverrideCursor();

    // Helper output lines: restored|backup|created|skipped|summary, tab separated.
    const auto lines = r.output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList f = line.split(QLatin1Char('\t'));
        const QString kind = f.value(0);
        if (kind == QLatin1String("restored"))
            log(tr("Restored %1").arg(f.value(1)));
        else if (kind == QLatin1String("backup"))
            log(tr("Current version of %1 kept as %2").arg(f.value(1), f.value(2)));
        else if (kind == QLatin1String("created"))
            log(tr("Recreated missing directory %1").arg(f.value(1)));
        else if (kind == QLatin1String("skipped"))
            log(tr("Skipped %1 (%2)").arg(f.value(1), f.value(2)));
        else if (kind == QLatin1String("summary") && f.size() >= 6)
            log(tr("Restore summary: %1 file(s), %2 director(ies), %3 symlink(s), "
                   "%4 skipped, %5 error(s).")
                    .arg(f[1], f[2], f[3], f[4], f[5]));
    }

    if (!r.ok) {
        m_tabs->setCurrentWidget(m_output);
        reportError(tr("Restore"), r.error);
        return;
    }
    log(tr("Restore from '%1' finished.").arg(s.name));
    statusBar()->showMessage(tr("Restore finished."), 5000);
}

void MainWindow::onAbout() {
    QMessageBox::about(this, tr("About OctoSnap"),
        tr("<b>OctoSnap %1</b><br>"
           "Snapshot manager for FFS/UFS on NetBSD, using fss(4).<br>"
           "Interface modeled after bectl and OctoPkgsrc.").arg(QApplication::applicationVersion()));
}

void MainWindow::reportError(const QString &title, const QString &message) {
    QMessageBox::critical(this, title, message);
    log(tr("Error: %1").arg(message));
}
