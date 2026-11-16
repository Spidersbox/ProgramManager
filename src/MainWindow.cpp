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
#include <QUrl>

#if QT_VERSION < 0x060000
  #include <QDesktopWidget>
#else
  #include <QScreen>
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
  setCentralWidget(mdiArea);

  // 3. Create actions and menu bar
  createActions();
  createMenuBar();
  menuBar()->setNativeMenuBar(false);


  loadGroupsFromIni();

  if(!groupCount)  // 4. Create the initial "Accessories" Group sub-window
  {
    QMdiSubWindow *groupWindow = new QMdiSubWindow();
    groupWindow->setWindowTitle("Accessories");
    groupWindow->resize(350, 250);

    // 5. Create an icon grid with drag-and-drop enabled
    QListWidget *iconGrid = new QListWidget();
    iconGrid->setViewMode(QListView::IconMode);
    iconGrid->setMovement(QListView::Snap);
    iconGrid->setDragEnabled(true);
    iconGrid->setAcceptDrops(true);
    iconGrid->setDropIndicatorShown(true);
    iconGrid->setDefaultDropAction(Qt::MoveAction);
//    groupList->setContextMenuPolicy(Qt::CustomContextMenu);

//    connect(groupList, SIGNAL(customContextMenuRequested(QPoint)), this,
//                       SLOT(on_groupContextMenu(QPoint)));

//    connect(groupList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
//                       SLOT(on_programDoubleClicked(QListWidgetItem*)));

//    connect(groupList, SIGNAL(fileDropped(GroupListWidget*, QString, QPoint)), this,
//                       SLOT(on_fileDropped(GroupListWidget*, QString, QPoint)));

    // Add a sample program item using this->style()
    QListWidgetItem *item = new QListWidgetItem
    (
      this->style()->standardIcon(QStyle::SP_ComputerIcon), "Terminal"
    );
    item->setData(Qt::UserRole, "x-terminal-emulator");
    iconGrid->addItem(item);

    // 6. Launch program on double-click
    connect(iconGrid, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
          this,     SLOT(on_programDoubleClicked(QListWidgetItem*)));

    // Put the grid into the sub-window, and add the sub-window to the MDI area
    groupWindow->setWidget(iconGrid);
    mdiArea->addSubWindow(groupWindow);
    groupWindow->show();
  }
}

//--------------------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
  shutdownClicked();
}

//-----------------------------------------------------------------------------------------
void MainWindow::createActions()
{
  /** main menu actions */
  newGroup = new QAction(tr("&Group"), this);
  newGroup->setToolTip(tr("Create a new Group"));
  newItem = new QAction(tr("&Item"), this);
  newItem->setToolTip(tr("Create a new Item"));




  /** main menu signals */
  connect(newGroup, SIGNAL(triggered()), this, SLOT(groupClicked()));
  connect(newItem, SIGNAL(triggered()), this, SLOT(itemClicked()));
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
  groupList->setViewMode(QListView::IconMode);
  groupList->setMovement(QListView::Snap);
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
  QMdiSubWindow *subWindow = mdiArea->addSubWindow(groupList);
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
void MainWindow::addItem(GroupListWidget *targetGroup, const QString &filePath,
                           const QString &customTitle, const QPoint &dropPos)
{
  if (!targetGroup || filePath.isEmpty()) return;

  QFileInfo fileInfo(filePath);
  QString title = customTitle.isEmpty() ? fileInfo.baseName() : customTitle;
  QString exePath = filePath;
  QString iconPath = filePath;
  QIcon itemIcon;

  // Bonus: If a Linux .desktop launcher was dropped, read its Name, Exec, and Icon
  if (fileInfo.suffix().toLower() == "desktop")
  {
    QSettings desktopIni(filePath, QSettings::IniFormat);
    desktopIni.beginGroup("Desktop Entry");
    if (customTitle.isEmpty())
    {
      title = desktopIni.value("Name", title).toString();
    }
    // Strip field codes like %u or %F from Exec command
    QString rawExec = desktopIni.value("Exec", exePath).toString();
    exePath = rawExec.section(' ', 0, 0);

    QString iconName = desktopIni.value("Icon").toString();
    if(!iconName.isEmpty())
    {
      iconPath = iconName;
      itemIcon = QIcon::fromTheme(iconName, QIcon(iconName));
    }
      desktopIni.endGroup();
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

  targetGroup->addItem(item);

  if (dropPos.x() >= 0 && dropPos.y() >= 0)
  {
    targetGroup->setPos(targetGroup->count() - 1, dropPos);
  }
}
//-----------------------------------------------------------------------------------------
// exit this app
void MainWindow::quitClicked()
{
//  saveSettings();
  saveGroupsToIni();
  qApp->quit();
}

//-----------------------------------------------------------------------------------------
// exit this app
void MainWindow::shutdownClicked()
{
//  saveSettings();
  saveGroupsToIni();
  qApp->quit();
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
    groupList->setViewMode(QListView::IconMode);
    groupList->setMovement(QListView::Free); // Lets icons sit at exact X,Y coordinates
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
    QMdiSubWindow *subWindow = mdiArea->addSubWindow(groupList);
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

        // Load icon (fallback to standard system icon if file doesn't exist on current OS)
        QIcon itemIcon;
        if (!iconPath.isEmpty())
        {
    // 1. Check Linux icon theme first (e.g., "utilities-terminal")
          if (QIcon::hasThemeIcon(iconPath))
          {
            itemIcon = QIcon::fromTheme(iconPath);
          }
    // 2. Next, check if iconPath is an actual image/icon file on disk
          else if (QFile::exists(iconPath))
          {
            QString ext = QFileInfo(iconPath).suffix().toLower();
            if (ext == "png" || ext == "ico" || ext == "svg" || ext == "xpm" || ext == "jpg" || ext == "gif")
            {
              itemIcon = QIcon(iconPath);
            }

            // 3. For .swf, .exe, documents, or folders, use the OS MIME/file icon provider
            if (itemIcon.isNull() || itemIcon.availableSizes().isEmpty())
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

      QString line = QString("%1,%2,%3,%4,%5X%6")
                               .arg(title)
                               .arg(exePath)
                               .arg(iconPath)
                               .arg(iconIndex)
                               .arg(pos.x())
                               .arg(pos.y());

      ini.setValue(QString("Icon%1").arg(i + 1), line);
    }

    ini.endGroup();
  }
}

//-----------------------------------------------------------------------------------------
void MainWindow::on_programDoubleClicked(QListWidgetItem *item)
{
  QString targetPath = item->data(ExePathRole).toString();
  if (targetPath.isEmpty()) return;

  QFileInfo info(targetPath);
  QString ext = info.suffix().toLower();

  // Common executable extensions (plus extensionless binaries on Linux)
  bool isRunnableApp = ext.isEmpty() || ext == "exe" || ext == "bat" ||
                        ext == "cmd" || ext == "sh" || ext == "appimage";

  if (!info.isDir() && info.isExecutable() && isRunnableApp)
  {
    QProcess::startDetached(targetPath, QStringList());
  }
  else
  {
    QDesktopServices::openUrl(QUrl::fromLocalFile(targetPath));
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
  if (!clickedItem) return; // Right-clicked empty space

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
    QString f = QFileDialog::getOpenFileName(&dialog, tr("Select Icon"), iconEdit->text(),
                                 tr("Icons & Images (*.png *.ico *.svg *.xpm *.exe);;All Files (*)"));
    if (!f.isEmpty()) iconEdit->setText(f);
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

    // Update the visual icon immediately
    if (!newIconPath.isEmpty())
    {
      QIcon customIcon = QIcon::fromTheme(newIconPath, QIcon(newIconPath));
      if (customIcon.isNull())
      {
        QFileIconProvider provider;
        customIcon = provider.icon(QFileInfo(newIconPath));
      }
      if (!customIcon.isNull())
      {
        item->setIcon(customIcon);
      }
    }
  }
}

