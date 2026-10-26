#include "qrconfig.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QXmlStreamReader>

extern QSettings g_settings;
extern QString logPathRoot;

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wlanapi.h>
#endif

QString currentWifiSsid()
{
#ifdef Q_OS_WIN
    HANDLE client = nullptr;
    DWORD negotiatedVersion = 0;
    if (WlanOpenHandle(2, nullptr, &negotiatedVersion, &client) != ERROR_SUCCESS) {
        return QString();
    }

    PWLAN_INTERFACE_INFO_LIST interfaces = nullptr;
    QString ssid;
    if (WlanEnumInterfaces(client, nullptr, &interfaces) == ERROR_SUCCESS) {
        for (DWORD i = 0; i < interfaces->dwNumberOfItems; ++i) {
            PWLAN_CONNECTION_ATTRIBUTES connection = nullptr;
            DWORD dataSize = 0;
            DWORD result = WlanQueryInterface(
                client,
                &interfaces->InterfaceInfo[i].InterfaceGuid,
                wlan_intf_opcode_current_connection,
                nullptr,
                &dataSize,
                reinterpret_cast<PVOID *>(&connection),
                nullptr
            );
            if (result == ERROR_SUCCESS &&
                connection->isState == wlan_interface_state_connected) {
                const DOT11_SSID &dot11Ssid =
                    connection->wlanAssociationAttributes.dot11Ssid;
                ssid = QString::fromUtf8(
                    reinterpret_cast<const char *>(dot11Ssid.ucSSID),
                    static_cast<int>(dot11Ssid.uSSIDLength)
                );
            }
            if (connection) {
                WlanFreeMemory(connection);
            }
            if (!ssid.isEmpty()) {
                break;
            }
        }
    }

    if (interfaces) {
        WlanFreeMemory(interfaces);
    }
    WlanCloseHandle(client, nullptr);
    return ssid;
#else
    return QString();
#endif
}

QString currentWifiPassword()
{
#ifdef Q_OS_WIN
    HANDLE client = nullptr;
    DWORD negotiatedVersion = 0;
    if (WlanOpenHandle(2, nullptr, &negotiatedVersion, &client) != ERROR_SUCCESS) {
        return QString();
    }
    PWLAN_INTERFACE_INFO_LIST interfaces = nullptr;
    QString password;
    if (WlanEnumInterfaces(client, nullptr, &interfaces) == ERROR_SUCCESS) {
        for (DWORD i = 0; i < interfaces->dwNumberOfItems && password.isEmpty(); ++i) {
            const GUID &guid = interfaces->InterfaceInfo[i].InterfaceGuid;
            PWLAN_CONNECTION_ATTRIBUTES connection = nullptr;
            DWORD dataSize = 0;
            if (WlanQueryInterface(client, &guid, wlan_intf_opcode_current_connection,
                    nullptr, &dataSize, reinterpret_cast<PVOID *>(&connection),
                    nullptr) != ERROR_SUCCESS) {
                continue;
            }
            if (connection->isState == wlan_interface_state_connected) {
                LPWSTR profileXml = nullptr;
                DWORD flags = WLAN_PROFILE_GET_PLAINTEXT_KEY;
                if (WlanGetProfile(client, &guid,
                        connection->strProfileName,
                        nullptr, &profileXml, &flags, nullptr) == ERROR_SUCCESS && profileXml) {
                    QXmlStreamReader xml(QString::fromWCharArray(profileXml));
                    while (!xml.atEnd()) {
                        if (xml.readNext() == QXmlStreamReader::StartElement &&
                            xml.name() == QLatin1String("keyMaterial")) {
                            password = xml.readElementText();
                            break;
                        }
                    }
                    WlanFreeMemory(profileXml);
                }
            }
            WlanFreeMemory(connection);
        }
    }
    if (interfaces) WlanFreeMemory(interfaces);
    WlanCloseHandle(client, nullptr);
    return password;
#else
    return QString();
#endif
}

QString qrConfigFilePath()
{
    return QDir::tempPath() + QDir::separator() + "chfs-qrcode.ini";
}

static QString singleLine(QString value)
{
    value.replace("\r", " ");
    value.replace("\n", " ");
    return value;
}

static QByteArray escapedNoticeText(QString value)
{
    QByteArray result = value.toHtmlEscaped().toUtf8();
    result.replace("`", "&#96;");
    result.replace("\r\n", "\n");
    result.replace("\r", "\n");
    result.replace("\n", "<br>");
    return result;
}

static QString defaultPageTitle()
{
    return QObject::tr("Cute HTTP File Server 局域网文件共享");
}

static QString defaultPageNotice()
{
    return QObject::tr("手机扫码即可访问共享文件；上传、下载大文件时建议使用浏览器。");
}

bool createQrConfig(QString &path, QString &errorMessage)
{
    QFile noticeFile(":/chfs_resource/qr-notice.html");
    if (!noticeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorMessage = QObject::tr("无法读取内置二维码页面资源。");
        return false;
    }

    QByteArray notice = noticeFile.readAll();
    notice.replace("\r", "");
    notice.replace("\n", " ");
    QByteArray wifiSsid = currentWifiSsid().toHtmlEscaped().toUtf8();
    wifiSsid.replace("`", "&#96;");
    notice.replace("__CHFS_WIFI_SSID__", wifiSsid);
    const bool wifiPasswordEnabled = g_settings.value("WIFI_PASSWORD_ENABLED", false).toBool();
    QByteArray wifiPassword = wifiPasswordEnabled
        ? currentWifiPassword().toHtmlEscaped().toUtf8() : QByteArray();
    wifiPassword.replace("`", "&#96;");
    notice.replace("__CHFS_WIFI_PASSWORD__", wifiPassword);
    notice.replace("__CHFS_WIFI_PASSWORD_ENABLED__", wifiPasswordEnabled ? "1" : "0");
    QString pageTitle = g_settings.value("HTML_TITLE", defaultPageTitle()).toString().trimmed();
    if (pageTitle.isEmpty()) pageTitle = defaultPageTitle();
    QString pageNotice = g_settings.value("HTML_NOTICE", defaultPageNotice()).toString().trimmed();
    if (pageNotice.isEmpty()) pageNotice = defaultPageNotice();
    notice.replace("__CHFS_PAGE_TITLE__", escapedNoticeText(pageTitle));
    notice.replace("__CHFS_USER_NOTICE__", escapedNoticeText(pageNotice));
    notice.replace(
        "__CHFS_FAST_IMAGE_PREVIEW__",
        g_settings.value("IMAGE_PREVIEW", true).toBool() &&
        g_settings.value("FAST_IMAGE_PREVIEW", true).toBool() ? "1" : "0"
    );
    notice.replace(
        "__CHFS_WEB_QR_ENABLED__",
        g_settings.value("WEB_QR_ENABLED", true).toBool() ? "1" : "0"
    );
    if (notice.contains('`')) {
        errorMessage = QObject::tr("二维码页面资源包含无效字符。");
        return false;
    }

    path = qrConfigFilePath();
    QFile configFile(path);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        errorMessage = QObject::tr("无法创建二维码临时配置：%1").arg(path);
        return false;
    }

    const auto writeLine = [&configFile](const QByteArray &key, const QString &value) {
        configFile.write(key);
        configFile.write(value.toUtf8());
        configFile.write("\r\n");
    };
    writeLine("port=", singleLine(g_settings.value("PORT", 80).toString()));
    writeLine("path=\"", singleLine(g_settings.value("PATH").toString()) + "\"");
    writeLine("allow=", singleLine(g_settings.value("ALLOW").toString()));
    writeLine("log=\"", singleLine(logPathRoot) + "\"");
    writeLine("html.title=", singleLine(pageTitle));
    writeLine(
        "image.preview=",
        g_settings.value("IMAGE_PREVIEW", true).toBool() ? "true" : "false"
    );
    writeLine(
        "folder.download=",
        g_settings.value("FOLDER_DOWNLOAD", "enable").toString()
    );
    writeLine("file.remove=", g_settings.value("FILE_REMOVE", "1").toString());
    writeLine(
        "session.timeout=",
        g_settings.value("SESSION_TIMEOUT", 30).toString()
    );
    const QString sslCert = singleLine(g_settings.value("SSL_CERT").toString());
    const QString sslKey = singleLine(g_settings.value("SSL_KEY").toString());
    if (!sslCert.isEmpty()) {
        writeLine("ssl.cert=\"", sslCert + "\"");
    }
    if (!sslKey.isEmpty()) {
        writeLine("ssl.key=\"", sslKey + "\"");
    }
    const QStringList rules =
        g_settings.value("RULE").toString().split('|', Qt::SkipEmptyParts);
    for (const QString &rule : rules) {
        writeLine("rule=", rule);
    }

    configFile.write("html.notice=`");
    configFile.write(notice);
    configFile.write("`\r\n");
    configFile.close();
    return true;
}

void removeQrConfig()
{
    QFile::remove(qrConfigFilePath());
}
