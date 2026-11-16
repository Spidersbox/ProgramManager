#ifndef GROUPLISTWIDGET_H
#define GROUPLISTWIDGET_H

#include <QListWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

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
};

#endif // GROUPLISTWIDGET_H

