#ifndef GROUPLISTWIDGET_H
#define GROUPLISTWIDGET_H

#include <QListWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QResizeEvent>
#include <QMdiSubWindow>

class GroupListWidget : public QListWidget
{
  Q_OBJECT

public:
  using QListWidget::QListWidget;

  void setPos(int row, const QPoint &pos)
  {
    setPositionForIndex(pos, model()->index(row, 0));
  }

signals:
  void fileDropped(GroupListWidget *targetGroup, const QString &filePath, const QPoint &dropPos);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override
  {
    if (event->mimeData()->hasUrls())
    {
      event->acceptProposedAction();
    }
    else
    {
      QListWidget::dragEnterEvent(event);
    }
  }

  void dragMoveEvent(QDragMoveEvent *event) override
  {
    if (event->mimeData()->hasUrls())
    {
      event->acceptProposedAction();
    }
    else
    {
      QListWidget::dragMoveEvent(event);
    }
  }

  void dropEvent(QDropEvent *event) override
  {
    if (event->mimeData()->hasUrls())
    {
      QList<QUrl> urls = event->mimeData()->urls();
      QPoint dropPos = event->pos();

      for (const QUrl &url : urls)
      {
        if (url.isLocalFile())
        {
          emit fileDropped(this, url.toLocalFile(), dropPos);
          dropPos += QPoint(20, 20);
        }
      }
      event->acceptProposedAction();
    }
    else
    {
      QListWidget::dropEvent(event);
    }
  }

  void resizeEvent(QResizeEvent *event) override
  {
    QListWidget::resizeEvent(event);

    QMdiSubWindow *sub = qobject_cast<QMdiSubWindow*>(parentWidget());
    if (sub)
    {
      Qt::WindowStates state = sub->windowState();
      // Do not reflow icons when minimized or maximized
      if (state.testFlag(Qt::WindowMinimized) || state.testFlag(Qt::WindowMaximized))
      {
        return;
      }
    }

    // Reflow icons cleanly to fit the new window width
    doItemsLayout();
  }

};

#endif // GROUPLISTWIDGET_H

