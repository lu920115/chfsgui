#include "appprotection.h"
#include "precompile.h"

#include <QCryptographicHash>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QUuid>

namespace {
const char *const enabledKey = "GUI_PROTECTION_ENABLED";
const char *const saltKey = "GUI_PROTECTION_SALT";
const char *const hashKey = "GUI_PROTECTION_HASH";

QString passwordHash(const QString &password, const QString &salt)
{
    QByteArray digest = QCryptographicHash::hash(
        (salt + "\n" + password).toUtf8(), QCryptographicHash::Sha256);
    for (int i = 0; i < 20000; ++i) {
        digest = QCryptographicHash::hash(digest, QCryptographicHash::Sha256);
    }
    return QString::fromLatin1(digest.toHex());
}

bool askPassword(QWidget *parent, const QString &title, const QString &label,
                 QString &password)
{
    bool accepted = false;
    password = QInputDialog::getText(parent, title, label,
                                     QLineEdit::Password, QString(), &accepted);
    return accepted;
}

bool verify(QWidget *parent)
{
    QString password;
    if (!askPassword(parent, QObject::tr("密码保护"),
                     QObject::tr("请输入 GUI 保护密码："), password)) {
        return false;
    }
    const QString salt = g_settings.value(saltKey).toString();
    if (passwordHash(password, salt) == g_settings.value(hashKey).toString()) {
        return true;
    }
    QMessageBox::warning(parent, QObject::tr("密码保护"),
                         QObject::tr("密码不正确。"));
    return false;
}

bool setNewPassword(QWidget *parent)
{
    QString first;
    if (!askPassword(parent, QObject::tr("设置保护密码"),
                     QObject::tr("请输入新的 GUI 保护密码："), first)) {
        return false;
    }
    if (first.isEmpty()) {
        QMessageBox::warning(parent, QObject::tr("密码保护"),
                             QObject::tr("密码不能为空。"));
        return false;
    }
    QString second;
    if (!askPassword(parent, QObject::tr("设置保护密码"),
                     QObject::tr("请再次输入密码："), second)) {
        return false;
    }
    if (first != second) {
        QMessageBox::warning(parent, QObject::tr("密码保护"),
                             QObject::tr("两次输入的密码不一致。"));
        return false;
    }
    const QString salt = QUuid::createUuid().toString();
    g_settings.setValue(saltKey, salt);
    g_settings.setValue(hashKey, passwordHash(first, salt));
    g_settings.setValue(enabledKey, true);
    g_settings.sync();
    return true;
}
}

namespace AppProtection {

bool isEnabled()
{
    return g_settings.value(enabledKey, false).toBool();
}

bool unlock(QWidget *parent)
{
    if (!isEnabled()) return true;
    while (isEnabled()) {
        if (verify(parent)) return true;
        if (QMessageBox::question(parent, QObject::tr("密码保护"),
                QObject::tr("是否重试？选择“否”将不打开主窗口。")) != QMessageBox::Yes) {
            return false;
        }
    }
    return true;
}

bool enable(QWidget *parent)
{
    if (isEnabled()) return true;
    if (!setNewPassword(parent)) return false;
    QMessageBox::information(parent, QObject::tr("密码保护"),
        QObject::tr("已开启保护。下次打开主窗口，以及关闭或修改保护时需要输入密码。\n该功能仅保护本机 GUI 设置，不替代共享账户权限。"));
    return true;
}

bool disable(QWidget *parent)
{
    if (!isEnabled()) return true;
    if (!verify(parent)) return false;
    g_settings.setValue(enabledKey, false);
    g_settings.remove(saltKey);
    g_settings.remove(hashKey);
    g_settings.sync();
    return true;
}

bool changePassword(QWidget *parent)
{
    if (!isEnabled() || !verify(parent)) return false;
    return setNewPassword(parent);
}

} // namespace AppProtection
