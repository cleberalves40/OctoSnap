#pragma once

#include <QMainWindow>

#include "FssBackend.h"

class QAction;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTabWidget;
class QTableWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refreshAll();
    void onTableSelectionChanged();
    void onCreate();
    void onDestroy();
    void onMount();
    void onUnmount();
    void onRestore();
    void onAbout();

private:
    void buildUi();
    void buildToolBar();
    void buildMenus();
    void populateSnapshotTable();
    void updateInformation(int index);
    void updateStatusBar();
    void updateThemeLabel();
    int selectedIndex() const;   // index into m_all, or -1
    bool mountInteractive(const QString &name);   // asks for a directory, then mounts
    void log(const QString &message);
    void reportError(const QString &title, const QString &message);

    FssBackend m_backend;

    // Table (top) and Information / Output / Help tabs (bottom), as in bectl.
    QTableWidget *m_snapTable = nullptr;
    QLineEdit *m_filter = nullptr;

    QTabWidget *m_tabs = nullptr;
    QPlainTextEdit *m_info = nullptr;
    QPlainTextEdit *m_output = nullptr;
    QPlainTextEdit *m_help = nullptr;

    // Status bar
    QLabel *m_themeLabel = nullptr;
    QLabel *m_countLabel = nullptr;

    QVector<SnapshotInfo> m_all;
    QVector<int> m_visible;      // table row -> index into m_all

    QAction *m_refreshAct = nullptr;
    QAction *m_createAct = nullptr;
    QAction *m_mountAct = nullptr;
    QAction *m_unmountAct = nullptr;
    QAction *m_restoreAct = nullptr;
    QAction *m_destroyAct = nullptr;
};
