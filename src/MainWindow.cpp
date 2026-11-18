#include <QApplication>
#include <QMainWindow>
#include <QInputDialog>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QListWidget>
#include <QProcess>
#include <QMenu>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStyle>
#include <QMenuBar>
#include <QIcon>
#include <QSettings>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QDesktopServices>
#include <QMimeData>
#include <QUrl>
#include <QRegularExpression>

#if QT_VERSION < 0x060000
  #include <QDesktopWidget>
#else
  #include <QScreen>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <QtWin> // Add 'QT += winextras' in your .pro file inside a win32 { } block
#endif


#include <QDebug>


#include "MainWindow.h"

//-----------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
  // 1. Configure THIS window directly (no separate QMainWindow variable!)
  setWindowTitle("Program Manager");
  resize(800, 600);

  // center form on screen
#if QT_VERSION < 0x060000
  QRect desktopRect = QApplication::desktop()->availableGeometry(this);
  QPoint center = desktopRect.center();
  move(center.x() - width() * 0.5, center.y() - height() * 0.5);
#else
  QScreen *screen = QGuiApplication::primaryScreen();
  QRect  screenGeometry = screen->geometry();
  int height = screenGeometry.height()/2;
  int width = screenGeometry.width()/2;
  move(width-340,height-285);
#endif

#ifndef Q_OS_MAC
    qApp->setWindowIcon(QIcon(":images/icon"));
    setWindowIcon(QIcon(":images/icon"));
#else
    setUnifiedTitleAndToolBarOnMac(true);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

  // 2. Initialize the MDI workspace area on THIS window FIRST
  mdiArea = new QMdiArea(this);
  mdiArea->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(mdiArea, SIGNAL(customContextMenuRequested(QPoint)), this,
                   SLOT(on_mdiContextMenu(QPoint)));

  setCentralWidget(mdiArea);

  // 3. Create actions and menu bar
  createActions();
  createMenuBar();
  menuBar()->setNativeMenuBar(false);
  createTrayIcon();


  loadGroupsFromIni();

}

//--------------------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
#ifndef Q_OS_MAC
  if (trayIcon && trayIcon->isVisible())
  {
    hide();
    event->ignore();
    return;
  }
#endif
  event->accept();
}

//-----------------------------------------------------------------------------------------
void MainWindow::createActions()
{
  /** main menu actions */
  newGroup = new QAction(tr("&Group"), this);
  newGroup->setToolTip(tr("Create a new Group"));

  newItem = new QAction(tr("&Item"), this);
  newItem->setToolTip(tr("Create a new Item"));

  fontSizeAction = new QAction(tr("Icon &Font Size..."), this);

  toggleHideAction = new QAction(tr("&Show / Hide"), this);
  toggleHideAction->setToolTip(tr("Show or hide"));

  quitAction = new QAction(tr("E&xit"), this);
  quitAction->setToolTip(tr("Quit application"));
  quitAction->setMenuRole(QAction::QuitRole);

  /** main menu signals */
  connect(newGroup, SIGNAL(triggered()), this, SLOT(groupClicked()));
  connect(newItem, SIGNAL(triggered()), this, SLOT(itemClicked()));
  connect(fontSizeAction, SIGNAL(triggered()), this, SLOT(fontSizeClicked()));
  connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
  connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
  connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(saveGroupsToIni()));
}

//-----------------------------------------------------------------------------------------
void MainWindow::createMenuBar()
{
#ifdef Q_OS_MAC
  /** Create a decoupled menu bar on Mac */
  /** which stays even if the window is closed */
  appMenuBar = new QMenuBar();
#else
  /** Get the main window's menu bar on other platforms */
  appMenuBar = menuBar();
#endif

  /** Configure the menus */
  QMenu *settings = appMenuBar->addMenu(tr("&Options"));
  QMenu *submenu=settings->addMenu("&New");
  submenu->addAction(newGroup);
  submenu->addAction(newItem);
  settings->addAction(fontSizeAction);

}

//----------------------------------------------------------------------------------------
void MainWindow::createTrayIcon()
{
  QMenu *trayIconMenu;
#ifndef Q_OS_MAC
  trayIcon = new QSystemTrayIcon(this);
  trayIconMenu = new QMenu(this);
  trayIcon->setContextMenu(trayIconMenu);
  trayIcon->setToolTip(tr("Program Manager"));
  trayIcon->setIcon(QIcon(":/images/icon"));

#else
  // Note: On Mac, the dock icon is used to provide the tray's functionality.
  MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
  trayIconMenu = dockIconHandler->dockMenu();
  dockIconHandler->setMainWindow((QMainWindow *)this);
#endif

  trayIconMenu->addAction(toggleHideAction);
  trayIconMenu->addSeparator();
//  trayIconMenu->addAction(openConfigAction);
//  trayIconMenu->addAction(optionsAction);

#ifndef Q_OS_MAC // This is built-in on Mac
//  trayIconMenu->addSeparator();
  trayIconMenu->addAction(quitAction);
#endif

  connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this,
                    SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
  trayIcon->show();

//  notificator=new Notificator(qApp->applicationName(), trayIcon);
}

//-----------------------------------------------------------------------------------------
void MainWindow::showNormalIfMinimized(bool fToggleHidden)
{
  // activateWindow() (sometimes) helps with keyboard focus on Windows
  if (isHidden())
  {
    show();
    activateWindow();
  }
  else if (isMinimized())
  {
    showNormal();
    activateWindow();
  }
  else if (!isActiveWindow())
  {
    // Bring to front if obscured by other windows
    raise();
    activateWindow();
  }
  else if(fToggleHidden)
    hide();
}

//-----------------------------------------------------------------------------------------
void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
  {
    toggleHidden();
  }
}

//-----------------------------------------------------------------------------------------
void MainWindow::toggleHidden()
{
  showNormalIfMinimized(true);
}

//-----------------------------------------------------------------------------------------
void MainWindow::fontSizeClicked()
{
  bool ok;
  int newSize = QInputDialog::getInt
  (
    this,
    tr("Icon Font Size"),
    tr("Font Size (pt):"),
    iconFontSize, // current value
    6,            // min size
    36,           // max size
    1,            // step
    &ok
  );

  if (!ok) return;

  iconFontSize = newSize;

  // Update all currently open group windows immediately
  for (QMdiSubWindow *sub : mdiArea->subWindowList())
  {
    GroupListWidget *groupList = static_cast<GroupListWidget*>(sub->widget());
    if (groupList)
    {
      QFont f = groupList->font();
      f.setPointSize(iconFontSize);
      groupList->setFont(f);
      groupList->doItemsLayout(); // Refresh grid spacing for new text size
    }
  }
}

//-----------------------------------------------------------------------------------------
void MainWindow::groupClicked()
{
  bool ok;
  QString groupName = QInputDialog::getText
  (
    this,
    tr("New Program Group"),
    tr("Group Name:"),
    QLineEdit::Normal,
    tr("New Group"),
    &ok
  );

  if (!ok || groupName.trimmed().isEmpty())
  {
    return; // User clicked Cancel or left it blank
  }

  // 1. Create the icon list (just like ui->listWidget in your MP3 player)
  GroupListWidget *groupList = new GroupListWidget();

  QFont f = groupList->font();
  f.setPointSize(iconFontSize);
  groupList->setFont(f);

  groupList->setViewMode(QListView::IconMode);
  groupList->setMovement(QListView::Snap);        // Pop icons into grid slots when added/dragged
  groupList->setResizeMode(QListView::Fixed);     // We will trigger layout manually in resizeEvent
  groupList->setGridSize(QSize(50, 45)); 
//  groupList->setGridSize(QSize(84, 72));          // Adjust cell width/height to taste

  groupList->setDragEnabled(true);
  groupList->setAcceptDrops(true);
  groupList->setDropIndicatorShown(true);
  groupList->setDefaultDropAction(Qt::MoveAction);
  groupList->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(groupList, SIGNAL(customContextMenuRequested(QPoint)), this,
                     SLOT(on_groupContextMenu(QPoint)));

  connect(groupList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
                     SLOT(on_programDoubleClicked(QListWidgetItem*)));

  connect(groupList, SIGNAL(fileDropped(GroupListWidget*, QString, QPoint)), this,
                     SLOT(on_fileDropped(GroupListWidget*, QString, QPoint)));

  //; 3. Wrap it in an MDI sub-window and show it
Qt::WindowFlags flags = Qt::SubWindow
                     | Qt::CustomizeWindowHint
                     | Qt::WindowTitleHint
                     | Qt::WindowMinimizeButtonHint;

  QMdiSubWindow *subWindow = mdiArea->addSubWindow(groupList, flags);
//  QMdiSubWindow *subWindow = mdiArea->addSubWindow(groupList);
  subWindow->setWindowTitle(groupName.trimmed());
  subWindow->resize(320, 240);
  subWindow->show();
}

//-----------------------------------------------------------------------------------------
void MainWindow::itemClicked()
{
  QMdiSubWindow *activeSub = mdiArea->activeSubWindow();
  if (!activeSub)
  {
    QMessageBox::information(this, tr("New Program Item"),
                                 tr("Please select or create a Program Group first."));
    return;
  }

  GroupListWidget *groupList = static_cast<GroupListWidget*>(activeSub->widget());
  if (!groupList) return;

  QString exePath = QFileDialog::getOpenFileName
  (
    this,
    tr("Select Program"),
    QDir::homePath(),
    tr("All Files (*)")
  );

  if (exePath.isEmpty()) return;

  QFileInfo fileInfo(exePath);
  bool ok;
  QString title = QInputDialog::getText
  (
    this,
    tr("Program Item Properties"),
    tr("Description:"),
    QLineEdit::Normal,
    fileInfo.baseName(),
    &ok
  );

  if (!ok || title.trimmed().isEmpty()) return;

  // Hand off the actual item creation to your shared helper
  addItem(groupList, exePath, title.trimmed());
}

//-----------------------------------------------------------------------------------------
QString MainWindow::resolveShortcutChain(const QString &initialPath)
{
  QString currentPath = initialPath;
  int maxDepth = 10; // Guard against circular symlinks

  while (maxDepth-- > 0)
  {
    QFileInfo info(currentPath);
    if (!info.isSymLink() && info.suffix().compare("lnk", Qt::CaseInsensitive) != 0)
    {
      break; // Reached the real target file
    }

    QString target = info.symLinkTarget();
    if (target.isEmpty() || target == currentPath)
    {
      break; // Broken link or self-reference
    }

    currentPath = target;
  }
  return currentPath;
}

//-----------------------------------------------------------------------------------------
void MainWindow::addItem(GroupListWidget *targetGroup, const QString &filePath,
                           const QString &customTitle, const QPoint &dropPos)
{
  if (!targetGroup || filePath.isEmpty()) return;

  QString openWith;
  QFileInfo originalInfo(filePath);
  QString resolvedPath = resolveShortcutChain(filePath);

  QFileInfo fileInfo(resolvedPath);
  // Use originalInfo for the title so "VMware.lnk" becomes "VMware", not "vmware"
  QString title = customTitle.isEmpty() ? originalInfo.completeBaseName() : customTitle;
  QString exePath = resolvedPath;
  QString iconPath = resolvedPath;
  QIcon itemIcon;

  // Bonus: If a Linux .desktop launcher was dropped, read its Name, Exec, and Icon
  if (fileInfo.suffix().toLower() == "desktop")
  {
    QFile desktopFile(resolvedPath);
    if (desktopFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QTextStream in(&desktopFile);
      bool inDesktopEntry = false;

      while (!in.atEnd())
      {
        QString line = in.readLine().trimmed();
        if (line.startsWith('['))
        {
          inDesktopEntry = (line == "[Desktop Entry]");
          continue;
        }
        if (!inDesktopEntry || line.startsWith('#') || !line.contains('='))
        {
          continue;
        }

        int eqPos = line.indexOf('=');
        QString key = line.left(eqPos).trimmed();
        QString val = line.mid(eqPos + 1).trimmed();

        if (key == "Name" && customTitle.isEmpty())
        {
          title = val;
        }
        else if (key == "Exec")
        {
          // Remove desktop field codes (%u, %U, %f, %F, %i, %c, %k)
          val.remove(QRegularExpression("%[fFuUick]"));
          exePath = val.trimmed();
        }
        else if (key == "Icon")
        {
          iconPath = val;
          itemIcon = QIcon::fromTheme(iconPath, QIcon(iconPath));
        }
      }
      desktopFile.close();
    }
  }

  if (itemIcon.isNull())
  {
    QFileIconProvider iconProvider;
    itemIcon = iconProvider.icon(fileInfo);
  }

  QListWidgetItem *item = new QListWidgetItem(itemIcon, title);
  item->setData(ExePathRole, exePath);
  item->setData(IconPathRole, iconPath);
  item->setData(IconIndexRole, 0);
  item->setData(OpenWithRole, openWith);

  targetGroup->addItem(item);

  if (dropPos.x() >= 0 && dropPos.y() >= 0)
  {
    targetGroup->setPos(targetGroup->count() - 1, dropPos);
  }
}

//-----------------------------------------------------------------------------------------
void MainWindow::loadGroupsFromIni()
{
  // Use a local INI file (behaves just like Borland's TIniFile)
  QString iniPath = QCoreApplication::applicationDirPath() + "/progman.ini";
  QSettings ini(iniPath, QSettings::IniFormat);

  // 1. Load MainWindow Form Options
  ini.beginGroup("Form Options");

  QStringList winSize = ini.value("Window Size", "800X600").toString().split('X', Qt::SkipEmptyParts);
  QStringList winPos  = ini.value("Window Location", "100X100").toString().split('X', Qt::SkipEmptyParts);
  if (winSize.size() == 2) resize(winSize[0].toInt(), winSize[1].toInt());
  if (winPos.size() == 2)  move(winPos[0].toInt(), winPos[1].toInt());
  iconFontSize = ini.value("IconFontSize", 9).toInt();

  ini.endGroup();

  // 2. Get total group count
  ini.beginGroup("Groups");
  groupCount = ini.value("number", 0).toInt();
  ini.endGroup();

  // 3. Loop through Group1 .. GroupN
  for (int g = 1; g <= groupCount; ++g)
  {
    ini.beginGroup(QString("Group%1").arg(g));
    QString groupName = ini.value("Name", "Group").toString();
    QStringList gSize = ini.value("Window Size", "320X240").toString().split('X', Qt::SkipEmptyParts);
    QStringList gPos  = ini.value("Window Location", "0X0").toString().split('X', Qt::SkipEmptyParts);
    int iconCount     = ini.value("Icons", 0).toInt();

    // Create the QListWidget for this group
    GroupListWidget *groupList = new GroupListWidget();

    QFont f = groupList->font();
    f.setPointSize(iconFontSize);
    groupList->setFont(f);

    groupList->setViewMode(QListView::IconMode);
    groupList->setMovement(QListView::Snap);        // Pop icons into grid slots when added/dragged
    groupList->setResizeMode(QListView::Fixed);     // We will trigger layout manually in resizeEvent
  groupList->setGridSize(QSize(50,45)); 
//    groupList->setGridSize(QSize(84, 72));          // Adjust cell width/height to taste

    groupList->setDragEnabled(true);
    groupList->setAcceptDrops(true);
    groupList->setDropIndicatorShown(true);
    groupList->setDefaultDropAction(Qt::MoveAction);
    groupList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(groupList, SIGNAL(customContextMenuRequested(QPoint)), this,
                       SLOT(on_groupContextMenu(QPoint)));

    connect(groupList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
                       SLOT(on_programDoubleClicked(QListWidgetItem*)));

    connect(groupList, SIGNAL(fileDropped(GroupListWidget*, QString, QPoint)), this,
                       SLOT(on_fileDropped(GroupListWidget*, QString, QPoint)));

    // Add the sub-window to mdiArea
    Qt::WindowFlags flags = Qt::SubWindow
                     | Qt::CustomizeWindowHint
                     | Qt::WindowTitleHint
                     | Qt::WindowMinimizeButtonHint;

    QMdiSubWindow *subWindow = mdiArea->addSubWindow(groupList, flags);
//    QMdiSubWindow *subWindow = mdiArea->addSubWindow(groupList);
    subWindow->setWindowTitle(groupName);
    if (gSize.size() == 2) subWindow->resize(gSize[0].toInt(), gSize[1].toInt());
    if (gPos.size() == 2)  subWindow->move(gPos[0].toInt(), gPos[1].toInt());

    // 4. Loop through Icon1 .. IconN
    for (int i = 1; i <= iconCount; ++i)
    {
      QString rawLine = ini.value(QString("Icon%1").arg(i)).toString();
      QStringList parts = rawLine.split(',');

      // Expecting: Title, ExePath, IconPath, IconIndex, PosXPos
      if (parts.size() >= 5)
      {
        QString title    = parts[0];
        QString exePath  = parts[1];
        QString iconPath = parts[2];
        int iconIndex    = parts[3].toInt();
        QStringList iPos = parts[4].split('X', Qt::SkipEmptyParts);
        QString openWith = (parts.size() >= 6) ? parts[5] : QString();

        // Load icon (fallback to standard system icon if file doesn't exist on current OS)
        QIcon itemIcon;
        if (!iconPath.isEmpty())
        {  // 1. Try multi-icon extraction first (Windows .dll/.exe or direct image)
          QList<QIcon> extracted = extractIconsFromFile(iconPath);
          if (iconIndex >= 0 && iconIndex < extracted.size())
          {
            itemIcon = extracted[iconIndex];
          }

          // 2. If not extracted, check Linux icon themes or OS file/MIME icons
          if (itemIcon.isNull() || itemIcon.availableSizes().isEmpty())
          {
            if (QIcon::hasThemeIcon(iconPath))
            {
              itemIcon = QIcon::fromTheme(iconPath);
            }
            else if (QFile::exists(iconPath))
            {
              QFileIconProvider provider;
              itemIcon = provider.icon(QFileInfo(iconPath));
            }
          }
        }
// 4. Final fallback if the file doesn't exist
        if (itemIcon.isNull() || itemIcon.availableSizes().isEmpty())
        {
          itemIcon = this->style()->standardIcon(QStyle::SP_FileIcon);
        }



        QListWidgetItem *item = new QListWidgetItem(itemIcon, title);
        item->setData(ExePathRole, exePath);
        item->setData(IconPathRole, iconPath);
        item->setData(IconIndexRole, iconIndex);
        item->setData(OpenWithRole, openWith);

        groupList->addItem(item);

        // Restore saved X,Y position inside the grid
        if (iPos.size() == 2)
        {
          groupList->setPos(groupList->count() - 1, QPoint(iPos[0].toInt(), iPos[1].toInt()));
        }
      }
    }

    subWindow->show();
    ini.endGroup();
  }
}

//-----------------------------------------------------------------------------------------
void MainWindow::saveGroupsToIni()
{
  QString iniPath = QCoreApplication::applicationDirPath() + "/progman.ini";
  QSettings ini(iniPath, QSettings::IniFormat);
  ini.clear(); // Remove old group entries before writing current state

  // 1. Save MainWindow Form Options
  ini.beginGroup("Form Options");

  ini.setValue("Window Size", QString("%1X%2").arg(width()).arg(height()));
  ini.setValue("Window Location", QString("%1X%2").arg(x()).arg(y()));
  ini.setValue("IconFontSize", iconFontSize);

  ini.endGroup();

  // 2. Get all sub-windows and write total count
  QList<QMdiSubWindow*> windows = mdiArea->subWindowList();
  ini.beginGroup("Groups");
  ini.setValue("number", windows.size());
  ini.endGroup();

  // 3. Loop through each sub-window and write [GroupX]
  for (int g = 0; g < windows.size(); ++g)
  {
    QMdiSubWindow *sub = windows[g];
    GroupListWidget *groupList = static_cast<GroupListWidget*>(sub->widget());
    if (!groupList) continue;

    ini.beginGroup(QString("Group%1").arg(g + 1));
    ini.setValue("Name", sub->windowTitle());
    ini.setValue("Window Size", QString("%1X%2").arg(sub->width()).arg(sub->height()));
    ini.setValue("Window Location", QString("%1X%2").arg(sub->x()).arg(sub->y()));
    ini.setValue("Icons", groupList->count());

    // 4. Loop through each icon in the group
    for (int i = 0; i < groupList->count(); ++i)
    {
      QListWidgetItem *item = groupList->item(i);
      QString title    = item->text();
      QString exePath  = item->data(ExePathRole).toString();
      QString iconPath = item->data(IconPathRole).toString();
      int iconIndex    = item->data(IconIndexRole).toInt();

      // Read the icon's current X,Y position inside the group window
      QPoint pos = groupList->visualItemRect(item).topLeft();

      QString openWith = item->data(OpenWithRole).toString();
      QString line = QString("%1,%2,%3,%4,%5X%6,%7")
                   .arg(title)
                   .arg(exePath)
                   .arg(iconPath)
                   .arg(iconIndex)
                   .arg(pos.x())
                   .arg(pos.y())
                   .arg(openWith);
      ini.setValue(QString("Icon%1").arg(i + 1), line);
    }

    ini.endGroup();
  }
}

//-----------------------------------------------------------------------------------------
void MainWindow::on_programDoubleClicked(QListWidgetItem *item)
{
  QString targetPath = item->data(ExePathRole).toString().trimmed();
  if (targetPath.isEmpty()) return;

  // 1. Manual "Open With" override from Properties (e.g. DOSBox .conf files)
  QString openWith = item->data(OpenWithRole).toString().trimmed();
  if (!openWith.isEmpty())
  {
    QStringList args;
    if (targetPath.endsWith(".conf", Qt::CaseInsensitive))
    {
      args << "-conf" << targetPath;
    }
    else
    {
      args << targetPath;
    }
    QProcess::startDetached(openWith, args);
    return;
  }

  // 2. If targetPath is a direct local file or folder on disk
  if (QFile::exists(targetPath))
  {
    QFileInfo info(targetPath);
    QString ext = info.suffix().toLower();
    bool isRunnableApp = ext.isEmpty() || ext == "exe" || ext == "bat" ||
                          ext == "cmd" || ext == "sh"  || ext == "appimage";

    if (!info.isDir() && info.isExecutable() && isRunnableApp)
    {
      QProcess::startDetached(targetPath, QStringList());
    }
    else
    {
      QDesktopServices::openUrl(QUrl::fromLocalFile(targetPath));
    }
    return;
  }

  // 3. Otherwise, treat targetPath as a full command line (Steam, Wine env, etc.)
  QStringList cmdParts = QProcess::splitCommand(targetPath);
  if (!cmdParts.isEmpty())
  {
    QString program = cmdParts.takeFirst();
    // Remove any trailing empty "" arguments left over by Wine .desktop files
    cmdParts.removeAll(QString());
    QProcess::startDetached(program, cmdParts);
  }
}

//-----------------------------------------------------------------------------------------
void MainWindow::on_fileDropped(GroupListWidget *targetGroup, const QString &filePath, const QPoint &dropPos)
{
  addItem(targetGroup, filePath, QString(), dropPos);
}

//-----------------------------------------------------------------------------------------
void MainWindow::on_groupContextMenu(const QPoint &pos)
{
  // sender() tells us which GroupListWidget was right-clicked
  GroupListWidget *groupList = static_cast<GroupListWidget*>(sender());
  if (!groupList) return;

  QListWidgetItem *clickedItem = groupList->itemAt(pos);
//  if (!clickedItem) return; // Right-clicked empty space
  if (!clickedItem)
  {
    QMdiSubWindow *subWindow = qobject_cast<QMdiSubWindow*>(groupList->parentWidget());
    if (!subWindow) return;

    QMenu groupMenu(this);
    QAction *newItemAct     = groupMenu.addAction(tr("&New Item..."));
    groupMenu.addSeparator();
    QAction *renameGroupAct = groupMenu.addAction(tr("&Rename Group..."));
    QAction *deleteGroupAct = groupMenu.addAction(tr("&Delete Group"));

    QAction *chosen = groupMenu.exec(groupList->mapToGlobal(pos));

    if (chosen == newItemAct)
    {
      itemClicked();
    }
    else if (chosen == renameGroupAct)
    {
      bool ok;
      QString newName = QInputDialog::getText
      (
        this, tr("Rename Program Group"), tr("Group Name:"),
               QLineEdit::Normal, subWindow->windowTitle(), &ok
      );
      if (ok && !newName.trimmed().isEmpty())
      {
        subWindow->setWindowTitle(newName.trimmed());
      }
    }
    else if (chosen == deleteGroupAct)
    {
      QMessageBox::StandardButton reply = QMessageBox::question
      (
        this, tr("Delete Program Group"),
        tr("Are you sure you want to delete the group '%1' and all its icons?")
        .arg(subWindow->windowTitle()),
        QMessageBox::Yes | QMessageBox::No
      );
      if (reply == QMessageBox::Yes)
      {
        mdiArea->removeSubWindow(subWindow);
        subWindow->deleteLater();
      }
    }
  return;
  }

  QMenu contextMenu(this);
  QAction *propAction   = contextMenu.addAction(tr("&Properties..."));
  contextMenu.addSeparator();
  QAction *deleteAction = contextMenu.addAction(tr("&Delete"));

  QAction *selected = contextMenu.exec(groupList->mapToGlobal(pos));

  if (selected == propAction)
  {
    editItemProperties(clickedItem);
  }
  else if (selected == deleteAction)
  {
    QMessageBox::StandardButton reply = QMessageBox::question
    (
      this, tr("Delete Item"),
      tr("Remove '%1' from this group?").arg(clickedItem->text()),
      QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes)
    {
      delete groupList->takeItem(groupList->row(clickedItem));
    }
  }
}

//-----------------------------------------------------------------------------------------
void MainWindow::editItemProperties(QListWidgetItem *item)
{
  if (!item) return;

  int currentIconIndex = item->data(IconIndexRole).toInt();
  QIcon selectedIcon = item->icon();

  QDialog dialog(this);
  dialog.setWindowTitle(tr("Program Item Properties"));
  dialog.resize(450, 220);

  QLineEdit *titleEdit    = new QLineEdit(item->text(), &dialog);
  QLineEdit *exeEdit      = new QLineEdit(item->data(ExePathRole).toString(), &dialog);
  QLineEdit *openWithEdit = new QLineEdit(item->data(OpenWithRole).toString(), &dialog);
  QLineEdit *iconEdit     = new QLineEdit(item->data(IconPathRole).toString(), &dialog);

  openWithEdit->setPlaceholderText(tr("(Optional) Program to open this file with"));

  QPushButton *browseExeBtn  = new QPushButton(tr("Browse..."), &dialog);
  QPushButton *browseOpenBtn = new QPushButton(tr("Browse..."), &dialog);
  QPushButton *browseIconBtn = new QPushButton(tr("Browse..."), &dialog);

  // Wire Browse buttons using small lambdas (or dedicated slots if you prefer)
  connect(browseExeBtn, &QPushButton::clicked, [&]()
  {
    QString f = QFileDialog::getOpenFileName(&dialog, tr("Select Target File"), exeEdit->text());
    if (!f.isEmpty()) exeEdit->setText(f);
  });

  connect(browseOpenBtn, &QPushButton::clicked, [&]()
  {
    QString f = QFileDialog::getOpenFileName(&dialog, tr("Select Application"), "/usr/bin");
    if (!f.isEmpty()) openWithEdit->setText(f);
  });

  connect(browseIconBtn, &QPushButton::clicked, [&]()
  {
//    QString f = QFileDialog::getOpenFileName(&dialog, tr("Select Icon"), iconEdit->text(),
//                                 tr("Icons & Images (*.png *.ico *.svg *.xpm *.exe);;All Files (*)"));
//    if (!f.isEmpty()) iconEdit->setText(f);
    QString currentPath = iconEdit->text().trimmed();
    if (pickIconDialog(currentPath, currentIconIndex, selectedIcon))
    {
      iconEdit->setText(currentPath);
    }
  });

  // Helper rows with Browse buttons
  auto makeBrowseRow = [](QLineEdit *edit, QPushButton *btn)
  {
    QHBoxLayout *row = new QHBoxLayout();
    row->addWidget(edit);
    row->addWidget(btn);
    return row;
  };

  QFormLayout *form = new QFormLayout();
  form->addRow(tr("Description:"), titleEdit);
  form->addRow(tr("Command / File:"), makeBrowseRow(exeEdit, browseExeBtn));
  form->addRow(tr("Open With:"), makeBrowseRow(openWithEdit, browseOpenBtn));
  form->addRow(tr("Icon Path:"), makeBrowseRow(iconEdit, browseIconBtn));

  QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttons, SIGNAL(accepted()), &dialog, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));

  QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
  mainLayout->addLayout(form);
  mainLayout->addWidget(buttons);

  if (dialog.exec() == QDialog::Accepted)
  {
    item->setText(titleEdit->text().trimmed());
    item->setData(ExePathRole, exeEdit->text().trimmed());
    item->setData(OpenWithRole, openWithEdit->text().trimmed());

    QString newIconPath = iconEdit->text().trimmed();
    item->setData(IconPathRole, newIconPath);
    item->setData(IconIndexRole, currentIconIndex);

    // Update the visual icon immediately
//    if (!newIconPath.isEmpty())
//    {
//      QIcon customIcon = QIcon::fromTheme(newIconPath, QIcon(newIconPath));
//      if (customIcon.isNull())
//      {
//        QFileIconProvider provider;
//        customIcon = provider.icon(QFileInfo(newIconPath));
//      }
//      if (!customIcon.isNull())
//      {
//        item->setIcon(customIcon);
//      }
//    }
 // If the user picked an icon from the grid, apply it immediately;
    // otherwise resolve whatever path/theme name they typed into iconEdit
    if (!selectedIcon.isNull() && newIconPath == item->data(IconPathRole).toString())
    {
      item->setIcon(selectedIcon);
    }
  }
}

//-----------------------------------------------------------------------------------------
QList<QIcon> MainWindow::extractIconsFromFile(const QString &filePath)
{
  QList<QIcon> icons;

#ifdef Q_OS_WIN
  std::wstring wPath = filePath.toStdWString();
  // Passing -1 for icon index returns the total number of icons embedded in the file
  UINT count = ExtractIconExW(wPath.c_str(), -1, nullptr, nullptr, 0);

  if (count > 0)
  {
    QVector<HICON> largeIcons(count, nullptr);
    UINT extracted = ExtractIconExW(wPath.c_str(), 0, largeIcons.data(), nullptr, count);

    for (UINT i = 0; i < extracted; ++i)
    {
      if (largeIcons[i])
      {
        QPixmap pix = QtWin::fromHICON(largeIcons[i]);
        icons.append(QIcon(pix));
        DestroyIcon(largeIcons[i]); // Clean up Win32 GDI handle
      }
    }
  }
#endif

  // Fallback for Ubuntu or standalone image files (.png, .ico, .svg)
  if (icons.isEmpty())
  {
    QIcon single(filePath);
    if (!single.isNull() && !single.availableSizes().isEmpty())
    {
      icons.append(single);
    }
  }
  return icons;
}

//-----------------------------------------------------------------------------------------
bool MainWindow::pickIconDialog(QString &iconPath, int &iconIndex, QIcon &chosenIcon)
{
  QDialog dlg(this);
  dlg.setWindowTitle(iconPath.isEmpty() ? tr("Select Icon") : iconPath);
  dlg.resize(380, 260);

  QListWidget *iconList = new QListWidget(&dlg);
  iconList->setViewMode(QListView::IconMode);
  iconList->setMovement(QListView::Static);
  iconList->setResizeMode(QListView::Adjust);
  iconList->setIconSize(QSize(32, 32));
  iconList->setGridSize(QSize(48, 48));

  auto populateGrid = [&](const QString &path)
  {
    iconList->clear();
    dlg.setWindowTitle(path);
    QList<QIcon> found = extractIconsFromFile(path);
    for (int i = 0; i < found.size(); ++i)
    {
      QListWidgetItem *it = new QListWidgetItem(found[i], QString());
      it->setData(Qt::UserRole, i); // Store the icon index
      iconList->addItem(it);
    }
    if (iconIndex >= 0 && iconIndex < iconList->count())
    {
      iconList->setCurrentRow(iconIndex);
    }
    else if (iconList->count() > 0)
    {
      iconList->setCurrentRow(0);
    }
  };
    

  if (!iconPath.isEmpty())
  {
    populateGrid(iconPath);
  }

  QPushButton *browseBtn = new QPushButton(tr("Browse..."), &dlg);
  QPushButton *okBtn     = new QPushButton(tr("OK"), &dlg);
  QPushButton *cancelBtn = new QPushButton(tr("Cancel"), &dlg);
  okBtn->setDefault(true);

  connect(browseBtn, &QPushButton::clicked, [&]()
  {
#ifdef Q_OS_WIN
    QString filter = tr("Icon Files (*.ico *.exe *.dll);;All Files (*)");
    QString startDir = "C:/Windows/System32";
#else
    QString filter = tr("Icon Files (*.png *.svg *.ico *.xpm);;All Files (*)");
    QString startDir = "/usr/share/pixmaps";
#endif
    QString chosenFile = QFileDialog::getOpenFileName(&dlg, tr("Browse for Icons"), startDir, filter);
    if (!chosenFile.isEmpty())
    {
      iconPath = chosenFile;
      iconIndex = 0;
      populateGrid(iconPath);
    }
  });

  // Double-clicking an icon in the grid selects it immediately
  connect(iconList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), &dlg, SLOT(accept()));
  connect(okBtn, SIGNAL(clicked()), &dlg, SLOT(accept()));
  connect(cancelBtn, SIGNAL(clicked()), &dlg, SLOT(reject()));

  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->addWidget(browseBtn);
  btnLayout->addStretch();
  btnLayout->addWidget(okBtn);
  btnLayout->addWidget(cancelBtn);

  QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
  mainLayout->addWidget(iconList);
  mainLayout->addLayout(btnLayout);
  if (dlg.exec() == QDialog::Accepted && iconList->currentItem())
  {
    iconIndex  = iconList->currentItem()->data(Qt::UserRole).toInt();
    chosenIcon = iconList->currentItem()->icon();
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------------------
void MainWindow::on_mdiContextMenu(const QPoint &pos)
{
  QMenu bgMenu(this);
  QAction *newGroupAct = bgMenu.addAction(tr("&New Group..."));

  if(bgMenu.exec(mdiArea->mapToGlobal(pos)) == newGroupAct)
  {
    groupClicked();
  }
}

//-----------------------------------------------------------------------------------------

