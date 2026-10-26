#include "updatemanager.h"
#include "precompile.h"
#include "appversion.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>
#include <QUrl>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {
const char *const repoKey = "UPDATE_REPOSITORY";
const char *const autoCheckKey = "UPDATE_AUTO_CHECK";

QVector<int> versionParts(const QString &text)
{
    const QRegularExpression re(QStringLiteral("(?i)^v?(\\d+)\\.(\\d+)(?:\\.(\\d+))?$"));
    const QRegularExpressionMatch match = re.match(text.trimmed());
    QVector<int> result;
    if (!match.hasMatch()) return result;
    result << match.captured(1).toInt() << match.captured(2).toInt();
    result << (match.captured(3).isEmpty() ? 0 : match.captured(3).toInt());
    return result;
}

int compareVersions(const QString &left, const QString &right)
{
    const QVector<int> a = versionParts(left);
    const QVector<int> b = versionParts(right);
    if (a.isEmpty() || b.isEmpty()) return 0;
    for (int i = 0; i < 3; ++i) {
        if (a[i] > b[i]) return 1;
        if (a[i] < b[i]) return -1;
    }
    return 0;
}

QNetworkRequest makeRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "CHFS-GUI-Updater/" APP_VERSION);
    request.setRawHeader("Accept", "application/vnd.github+json, application/octet-stream");
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    return request;
}
}

UpdateManager::UpdateManager(QWidget *parent)
    : QObject(parent), _network(new QNetworkAccessManager(this))
{
}

QWidget *UpdateManager::parentWidget() const
{
    return qobject_cast<QWidget *>(parent());
}

QString UpdateManager::normalizeRepository(const QString &value)
{
    QString repo = value.trimmed();
    repo.replace('\\', '/');
    const QRegularExpression prefix(QStringLiteral("(?i)^https?://github\\.com/"));
    repo.remove(prefix);
    if (repo.endsWith('/')) repo.chop(1);
    if (repo.endsWith(".git", Qt::CaseInsensitive)) repo.chop(4);
    const QRegularExpression valid(QStringLiteral("^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$"));
    return valid.match(repo).hasMatch() ? repo : QString();
}

QString UpdateManager::repository()
{
    return g_settings.value(repoKey, QStringLiteral("lu920115/chfs-gui-qrcode")).toString();
}

bool UpdateManager::autoCheckEnabled() const
{
    return g_settings.value(autoCheckKey, true).toBool();
}

void UpdateManager::setAutoCheckEnabled(bool enabled)
{
    g_settings.setValue(autoCheckKey, enabled);
}

void UpdateManager::configureRepository()
{
    bool accepted = false;
    const QString current = repository().isEmpty()
        ? QStringLiteral("https://github.com/所有者/仓库名") : repository();
    const QString input = QInputDialog::getText(
        parentWidget(), QObject::tr("设置 GitHub 更新仓库"),
        QObject::tr("请输入公开仓库地址或“所有者/仓库名”：\n程序将读取该仓库最新 Release 中的 EXE。"),
        QLineEdit::Normal, current, &accepted);
    if (!accepted) return;
    const QString repo = normalizeRepository(input);
    if (repo.isEmpty()) {
        QMessageBox::warning(parentWidget(), QObject::tr("软件更新"),
            QObject::tr("仓库地址无效。示例：https://github.com/owner/repository"));
        return;
    }
    g_settings.setValue(repoKey, repo);
    g_settings.sync();
    QMessageBox::information(parentWidget(), QObject::tr("软件更新"),
        QObject::tr("更新仓库已保存：%1\n启动时自动检查：%2")
            .arg(repo, autoCheckEnabled() ? QObject::tr("开启") : QObject::tr("关闭")));
}

void UpdateManager::checkForUpdates(bool silent)
{
    if (_stage != Idle) {
        if (!silent) QMessageBox::information(parentWidget(), QObject::tr("软件更新"),
            QObject::tr("正在检查或下载更新，请稍候。"));
        return;
    }
    if (repository().isEmpty()) {
        if (silent) return;
        configureRepository();
        if (repository().isEmpty()) return;
    }

    _silent = silent;
    _cancelled = false;
    _stage = Checking;
    const QUrl url(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(repository()));
    QNetworkReply *reply = _network->get(makeRequest(url));
    _reply = reply;
    QTimer::singleShot(30000, reply, [reply]() {
        if (reply->isRunning()) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (_stage != Checking || _reply != reply) return;
        const QByteArray payload = reply->readAll();
        const QString error = reply->errorString();
        const bool failed = reply->error() != QNetworkReply::NoError;
        reply->deleteLater();
        _reply = nullptr;
        if (failed) {
            fail(QObject::tr("无法检查 GitHub 更新：%1").arg(error));
            return;
        }

        const QJsonObject release = QJsonDocument::fromJson(payload).object();
        const QString tag = release.value("tag_name").toString();
        _newVersion = tag;
        _releasePage = release.value("html_url").toString();
        if (versionParts(tag).isEmpty()) {
            fail(QObject::tr("最新 Release 的版本号无法识别。请使用 v1.6、v1.6.1 这类标签。"));
            return;
        }
        if (compareVersions(tag, QStringLiteral(APP_VERSION)) <= 0) {
            const bool showCurrent = !_silent;
            reset();
            if (showCurrent) QMessageBox::information(parentWidget(), QObject::tr("软件更新"),
                QObject::tr("当前已是最新版本 v%1。").arg(QStringLiteral(APP_VERSION)));
            return;
        }

        int bestScore = -1;
        _assetName.clear();
        _assetUrl.clear();
        const QJsonArray assets = release.value("assets").toArray();
        for (const QJsonValue &value : assets) {
            const QJsonObject asset = value.toObject();
            const QString name = asset.value("name").toString();
            const QString url = asset.value("browser_download_url").toString();
            if (!name.endsWith(".exe", Qt::CaseInsensitive) || url.isEmpty()) continue;
            int score = 0;
            if (name.contains("CHFS", Qt::CaseInsensitive)) score += 4;
            if (name.contains("GUI", Qt::CaseInsensitive)) score += 2;
            if (name.contains(tag, Qt::CaseInsensitive)) score += 1;
            if (score > bestScore) {
                bestScore = score;
                _assetName = name;
                _assetUrl = url;
            }
        }
        if (_assetUrl.isEmpty()) {
            fail(QObject::tr("发现新版本 %1，但 Release 中没有 EXE 附件。").arg(tag));
            return;
        }

        const QRegularExpression hashRe(
            QStringLiteral("(?i)sha-?256[^0-9a-f]{0,30}([0-9a-f]{64})"));
        const QRegularExpressionMatch hashMatch = hashRe.match(release.value("body").toString());
        _expectedHash = hashMatch.hasMatch() ? hashMatch.captured(1).toLower() : QString();

        const QString message = QObject::tr(
            "发现新版本 %1（当前 v%2）。\n附件：%3\n\n是否现在从 GitHub 下载并更新？服务将在安装更新时短暂停止。")
            .arg(tag, QStringLiteral(APP_VERSION), _assetName);
        if (QMessageBox::question(parentWidget(), QObject::tr("发现新版本"), message,
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes) {
            reset();
            return;
        }
        _silent = false;
        downloadRelease();
    });
}

void UpdateManager::downloadRelease()
{
    _stage = Downloading;
    _cancelled = false;
    QString safeVersion = _newVersion;
    safeVersion.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), QStringLiteral("_"));
    _downloadPath = QDir::temp().filePath(QStringLiteral("CHFS-GUI-update-%1.exe").arg(safeVersion));
    const QString partPath = _downloadPath + ".part";
    QFile::remove(partPath);
    _downloadFile = new QFile(partPath, this);
    if (!_downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QObject::tr("无法创建更新下载文件。"));
        return;
    }

    _progress = new QProgressDialog(parentWidget());
    _progress->setWindowTitle(QObject::tr("下载更新"));
    _progress->setLabelText(QObject::tr("正在从 GitHub 下载 %1……").arg(_assetName));
    _progress->setCancelButtonText(QObject::tr("取消"));
    _progress->setRange(0, 100);
    _progress->setValue(0);
    _progress->setWindowModality(Qt::WindowModal);
    _progress->setAutoClose(false);
    _progress->setAutoReset(false);
    _progress->show();

    QNetworkReply *reply = _network->get(makeRequest(QUrl(_assetUrl)));
    _reply = reply;
    QTimer::singleShot(180000, reply, [reply]() {
        if (reply->isRunning()) reply->abort();
    });
    connect(_progress, &QProgressDialog::canceled, this, [this]() {
        _cancelled = true;
        if (_reply) _reply->abort();
    });
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        if (_downloadFile && _downloadFile->isOpen()) _downloadFile->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::downloadProgress, this,
        [this](qint64 received, qint64 total) {
            if (!_progress) return;
            if (total > 0) {
                _progress->setRange(0, 100);
                _progress->setValue(static_cast<int>(received * 100 / total));
            } else {
                _progress->setRange(0, 0);
            }
        });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (_stage != Downloading || _reply != reply) return;
        const QString error = reply->errorString();
        const bool failed = reply->error() != QNetworkReply::NoError;
        reply->deleteLater();
        _reply = nullptr;
        if (_downloadFile) {
            _downloadFile->flush();
            _downloadFile->close();
        }
        if (_progress) {
            _progress->hide();
            _progress->deleteLater();
            _progress = nullptr;
        }
        if (_cancelled) {
            if (_downloadFile) QFile::remove(_downloadFile->fileName());
            reset();
            return;
        }
        if (failed) {
            const QString path = _downloadFile ? _downloadFile->fileName() : QString();
            QFile::remove(path);
            fail(QObject::tr("更新下载失败：%1").arg(error));
            return;
        }
        const QString partPath = _downloadFile->fileName();
        _downloadFile->deleteLater();
        _downloadFile = nullptr;
        QFile::remove(_downloadPath);
        if (!QFile::rename(partPath, _downloadPath)) {
            fail(QObject::tr("无法保存更新程序。"));
            return;
        }
        installDownloaded();
    });
}

QString UpdateManager::fileHash(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) return QString();
    return QString::fromLatin1(hash.result().toHex());
}

void UpdateManager::installDownloaded()
{
    QFile probe(_downloadPath);
    if (!probe.open(QIODevice::ReadOnly) || probe.read(2) != "MZ") {
        QFile::remove(_downloadPath);
        fail(QObject::tr("下载内容不是有效的 Windows EXE，已取消更新。"));
        return;
    }
    probe.close();
    const QString actualHash = fileHash(_downloadPath);
    if (!_expectedHash.isEmpty() && actualHash.compare(_expectedHash, Qt::CaseInsensitive) != 0) {
        QFile::remove(_downloadPath);
        fail(QObject::tr("更新文件的 SHA-256 与 Release 说明不一致，已取消更新。"));
        return;
    }

    const QString message = QObject::tr(
        "新版本 %1 已下载并通过基本检查。\nSHA-256：%2\n\n点击“是”后，程序会关闭、替换旧文件并自动重新启动。")
        .arg(_newVersion, actualHash);
    if (QMessageBox::question(parentWidget(), QObject::tr("安装更新"), message,
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes) {
        reset();
        return;
    }

    const QString target = QCoreApplication::applicationFilePath();
    const QStringList arguments = QStringList()
        << QStringLiteral("--apply-update") << target
        << QString::number(QCoreApplication::applicationPid());
    if (!QProcess::startDetached(_downloadPath, arguments)) {
        fail(QObject::tr("无法启动更新程序，请手动替换 EXE。"));
        return;
    }
    _stage = Idle;
    if (g_chfsProcess.state() != QProcess::NotRunning) {
        g_chfsProcess.kill();
        g_chfsProcess.waitForFinished(5000);
    }
    QCoreApplication::quit();
}

void UpdateManager::reset()
{
    if (_reply) {
        _reply->disconnect(this);
        _reply->deleteLater();
        _reply = nullptr;
    }
    if (_progress) {
        _progress->deleteLater();
        _progress = nullptr;
    }
    if (_downloadFile) {
        _downloadFile->deleteLater();
        _downloadFile = nullptr;
    }
    _stage = Idle;
    _silent = false;
    _cancelled = false;
}

void UpdateManager::fail(const QString &message)
{
    const bool show = !_silent;
    reset();
    if (show) QMessageBox::warning(parentWidget(), QObject::tr("软件更新"), message);
}

int UpdateManager::applyUpdate(const QString &targetPath, quint32 oldProcessId)
{
#ifdef Q_OS_WIN
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, oldProcessId);
    if (process) {
        WaitForSingleObject(process, 30000);
        CloseHandle(process);
    }
    const QString backup = targetPath + ".update-backup";
    QFile::remove(backup);
    bool copied = false;
    for (int attempt = 0; attempt < 40 && !copied; ++attempt) {
        if (QFile::exists(targetPath) && !QFile::exists(backup) &&
            !QFile::rename(targetPath, backup)) {
            QThread::msleep(500);
            continue;
        }
        copied = QFile::copy(QCoreApplication::applicationFilePath(), targetPath);
        if (!copied) {
            // 保留备份，绝不能在恢复失败后的下一轮尝试中删除旧程序。
            QFile::remove(targetPath);
            if (QFile::exists(backup) && !QFile::exists(targetPath)) {
                QFile::rename(backup, targetPath);
            }
            QThread::msleep(500);
        }
    }
    if (!copied) {
        if (QFile::exists(backup) && !QFile::exists(targetPath)) {
            QFile::rename(backup, targetPath);
        }
        QMessageBox::critical(nullptr, QObject::tr("软件更新"),
            QObject::tr("无法替换旧程序。请关闭所有 CHFS 窗口后手动替换 EXE。旧程序备份（如存在）会保留在原目录。"));
        return 1;
    }
    QFile::remove(backup);
    if (!QProcess::startDetached(targetPath, QStringList())) {
        QMessageBox::critical(nullptr, QObject::tr("软件更新"),
            QObject::tr("更新文件已就位，但无法自动启动，请手动打开程序。"));
        return 1;
    }
    return 0;
#else
    Q_UNUSED(targetPath)
    Q_UNUSED(oldProcessId)
    return 1;
#endif
}
