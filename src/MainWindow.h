#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QMdiArea>
#include <QListWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QSystemTrayIcon>
#include <QCloseEvent>

#include "GroupListWidget.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT  // Required for SIGNAL/SLOT macros to work

protected:
 void closeEvent(QCloseEvent *event) override;

public:
  explicit MainWindow(QWidget *parent = nullptr);

  enum ItemDataRole
  {
    ExePathRole   = Qt::UserRole + 1,
    IconPathRole  = Qt::UserRole + 2,
    IconIndexRole = Qt::UserRole + 3,
    OpenWithRole  = Qt::UserRole + 4 
  };

  QMenuBar *appMenuBar;
  QMdiArea *mdiArea;
  QMenu *trayIconMenu;
  QSystemTrayIcon *trayIcon;

  int groupCount;
  int iconFontSize = 9;

  void loadGroupsFromIni();
//  void saveGroupsToIni();
  void createActions();
  void createMenuBar();
  void createTrayIcon();
  void editItemProperties(QListWidgetItem *item);

 QString resolveShortcutChain(const QString &initialPath);

  QList<QIcon> extractIconsFromFile(const QString &filePath);

  void addItem(GroupListWidget *targetGroup,
               const QString &filePath,
               const QString &customTitle = QString(),
               const QPoint &dropPos = QPoint(-1, -1));

  bool pickIconDialog(QString &iconPath, int &iconIndex, QIcon &chosenIcon);


  QAction *newGroup;
  QAction *newItem;
  QAction *fontSizeAction;
  QAction *toggleHideAction;
  QAction *quitAction;

private slots:
  // Your classic event handler function
  void on_programDoubleClicked(QListWidgetItem *item);
  void on_fileDropped(GroupListWidget *targetGroup, const QString &filePath, const QPoint &dropPos);
  void on_groupContextMenu(const QPoint &pos);
  void on_mdiContextMenu(const QPoint &pos);
  void groupClicked();
  void itemClicked();
//  void quitClicked();
//  void shutdownClicked();
  void fontSizeClicked();
  void saveGroupsToIni();
  void showNormalIfMinimized(bool fToggleHidden = false);
  void toggleHidden();
  void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

};

#endif // MAINWINDOW_H


