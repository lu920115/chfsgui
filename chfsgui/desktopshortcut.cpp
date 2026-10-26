#include "desktopshortcut.h"
#include "appversion.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <winnls.h>
#endif

namespace {

#ifdef Q_OS_WIN
bool createLink(const QString &linkPath, const QString &arguments,
                const QString &description, QString *errorMessage)
{
    IShellLinkW *link = nullptr;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_IShellLinkW, reinterpret_cast<void **>(&link));
    if (FAILED(result)) {
        if (errorMessage) *errorMessage = QObject::tr("无法创建桌面快捷方式组件。");
        return false;
    }

    const QString executable = QCoreApplication::applicationFilePath();
    const QString workingDirectory = QFileInfo(executable).absolutePath();
    result = link->SetPath(reinterpret_cast<LPCWSTR>(executable.utf16()));
    if (SUCCEEDED(result)) result = link->SetArguments(reinterpret_cast<LPCWSTR>(arguments.utf16()));
    if (SUCCEEDED(result)) result = link->SetWorkingDirectory(reinterpret_cast<LPCWSTR>(workingDirectory.utf16()));
    if (SUCCEEDED(result)) result = link->SetDescription(reinterpret_cast<LPCWSTR>(description.utf16()));
    if (SUCCEEDED(result)) result = link->SetIconLocation(reinterpret_cast<LPCWSTR>(executable.utf16()), 0);

    IPersistFile *persist = nullptr;
    if (SUCCEEDED(result)) {
        result = link->QueryInterface(IID_IPersistFile, reinterpret_cast<void **>(&persist));
    }
    if (SUCCEEDED(result)) {
        result = persist->Save(reinterpret_cast<LPCWSTR>(linkPath.utf16()), TRUE);
        persist->Release();
    }
    link->Release();
    if (FAILED(result)) {
        if (errorMessage) *errorMessage = QObject::tr("保存快捷方式失败：%1").arg(linkPath);
        return false;
    }
    return true;
}
#endif

}

namespace DesktopShortcut {

bool create(QString *errorMessage)
{
#ifdef Q_OS_WIN
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktop.isEmpty()) {
        if (errorMessage) *errorMessage = QObject::tr("找不到当前用户的桌面目录。");
        return false;
    }
    desktop = QDir::cleanPath(desktop);

    const HRESULT initialization = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool canInitialize = SUCCEEDED(initialization);
    if (!canInitialize && initialization != RPC_E_CHANGED_MODE) {
        if (errorMessage) *errorMessage = QObject::tr("无法初始化 Windows 快捷方式服务。");
        return false;
    }

    const QString programLink = QDir(desktop).filePath(QStringLiteral(APP_TITLE) + ".lnk");
    const QString webLink = QDir(desktop).filePath(QStringLiteral("CHFS 共享网页.lnk"));
    bool ok = createLink(programLink, QString(), QStringLiteral(APP_TITLE), errorMessage);
    if (ok) {
        ok = createLink(webLink, QStringLiteral("--open-web"),
                        QObject::tr("启动 CHFS 并打开当前共享网页"), errorMessage);
    }
    if (canInitialize) CoUninitialize();
    return ok;
#else
    if (errorMessage) *errorMessage = QObject::tr("桌面快捷方式仅支持 Windows。");
    return false;
#endif
}

}
