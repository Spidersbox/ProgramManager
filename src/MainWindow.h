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

#include "GroupListWidget.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT  // Required for SIGNAL/SLOT macros to work

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

  int groupCount;

  void closeEvent(QCloseEvent *event);
  void loadGroupsFromIni();
  void saveGroupsToIni();
  void createActions();
  void createMenuBar();
  void editItemProperties(QListWidgetItem *item);

  void addItem(GroupListWidget *targetGroup,
               const QString &filePath,
               const QString &customTitle = QString(),
               const QPoint &dropPos = QPoint(-1, -1));


  QAction *newGroup;
  QAction *newItem;

private slots:
  // Your classic event handler function
  void on_programDoubleClicked(QListWidgetItem *item);
  void on_fileDropped(GroupListWidget *targetGroup, const QString &filePath, const QPoint &dropPos);
  void on_groupContextMenu(const QPoint &pos);
  void groupClicked();
  void itemClicked();
  void quitClicked();
  void shutdownClicked();

};

#endif // MAINWINDOW_H


