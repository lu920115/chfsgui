#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QPointer>
#include <QString>

class QWidget;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;
class QFile;

class UpdateManager : public QObject
{
    Q_OBJECT
public:
    explicit UpdateManager(QWidget *parent = nullptr);

    void checkForUpdates(bool silent);
    void configureRepository();
    bool autoCheckEnabled() const;
    void setAutoCheckEnabled(bool enabled);

    static QString repository();
    static QString normalizeRepository(const QString &value);
    static int applyUpdate(const QString &targetPath, quint32 oldProcessId);

private:
    enum Stage { Idle, Checking, Downloading };

    QWidget *parentWidget() const;
    void reset();
    void fail(const QString &message);
    void downloadRelease();
    void installDownloaded();
    QString fileHash(const QString &path) const;

    QNetworkAccessManager *_network;
    QPointer<QNetworkReply> _reply;
    QPointer<QProgressDialog> _progress;
    QFile *_downloadFile = nullptr;
    Stage _stage = Idle;
    bool _silent = false;
    bool _cancelled = false;
    QString _newVersion;
    QString _releasePage;
    QString _expectedHash;
    QString _assetName;
    QString _assetUrl;
    QString _downloadPath;
};

#endif // UPDATEMANAGER_H
