#ifndef MONITORWGT_H
#define MONITORWGT_H

#include "uicomponents/qrpopup.h"
#include <QTextBrowser>
#include <QStringList>
#include <QMouseEvent>
#include <QListWidget>
#include <QScreen>
#include <QGuiApplication>

bool hoverQrEnabled();

class MyTextBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit MyTextBrowser(QWidget *parent = 0):QTextBrowser(parent)
    {
    }

    void mouseMoveEvent(QMouseEvent * e) override
    {
        QTextBrowser::mouseMoveEvent(e);

        if (!hoverQrEnabled()) {
            QRPopup::instance().hide();
            _oldAnchor.clear();
            return;
        }

        QString url = anchorAt(e->pos());
        if( url.isEmpty() ){//关闭QR
            QRPopup::instance().hide();
        }else{
            QRPopup &popup = QRPopup::instance();
            if( _oldAnchor != url ){//更新QR
                popup.encode(url);
            }
            // 默认放在鼠标左上方，并限制在当前屏幕可用区域内。
            const QPoint cursor = e->globalPos();
            QPoint position = cursor - QPoint(popup.width() + 16, popup.height() + 16);
            QScreen *screen = QGuiApplication::screenAt(cursor);
            if (!screen) screen = QGuiApplication::primaryScreen();
            if (screen) {
                const QRect area = screen->availableGeometry();
                position.setX(qBound(area.left(), position.x(),
                                     area.right() - popup.width() + 1));
                position.setY(qBound(area.top(), position.y(),
                                     area.bottom() - popup.height() + 1));
            }
            popup.move(position);
            popup.show();
            popup.raise();
        }

        _oldAnchor = url;
    }

    void leaveEvent(QEvent *event) override
    {
        QTextBrowser::leaveEvent(event);
        QRPopup::instance().hide();
        _oldAnchor.clear();
    }

    void mouseReleaseEvent(QMouseEvent * e) override
    {
        QTextBrowser::mouseReleaseEvent(e);

        QRPopup::instance().hide();
        _oldAnchor = "";
    }
private:
    QString _oldAnchor;
signals:

public slots:
};


class QMenu;
class LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = 0);
    void paintEvent(QPaintEvent *) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    QListWidget* logList;
private:
    void createActions();
    QMenu* menu;
signals:

public slots:

};


class MonitorWgt : public QWidget
{
    Q_OBJECT
public:
    explicit MonitorWgt(QWidget *parent = 0);
    void paintEvent(QPaintEvent *) override;
private:
    QStringList _websiteCandidates;
    QString _selectedWebsiteUrl;
signals:
    void sigWebsiteUrl(const QString &url);

public slots:
};

#endif // MONITORWGT_H
