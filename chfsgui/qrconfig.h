#ifndef QRCONFIG_H
#define QRCONFIG_H

#include <QString>

QString qrConfigFilePath();
bool createQrConfig(QString &path, QString &errorMessage);
void removeQrConfig();

#endif // QRCONFIG_H
