#ifndef APPPROTECTION_H
#define APPPROTECTION_H

class QWidget;

namespace AppProtection {
bool isEnabled();
bool unlock(QWidget *parent);
bool enable(QWidget *parent);
bool disable(QWidget *parent);
bool changePassword(QWidget *parent);
}

#endif // APPPROTECTION_H
