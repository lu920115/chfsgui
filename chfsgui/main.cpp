#include "mainwindow.h"
#include "updatemanager.h"
#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include "processdecorator.h"
#include "qtsingleapplication/qtsingleapplication.h"
#include <QStandardPaths>
#include <QDir>
#include <QTimer>

QString logPathRoot;

int main(int argc, char *argv[])
{
    QtSingleApplication a(argc, argv);
    const QStringList arguments = a.arguments();

    // 更新模式必须先于单实例检测，否则新进程会把消息发给旧进程并退出。
    if (arguments.size() >= 4 && arguments.at(1) == QStringLiteral("--apply-update")) {
        bool validPid = false;
        const quint32 oldProcessId = arguments.at(3).toUInt(&validPid);
        if (!validPid || oldProcessId == 0) return 2;
        return UpdateManager::applyUpdate(arguments.at(2), oldProcessId);
    }

    const bool openWeb = arguments.contains(QStringLiteral("--open-web"));
    if (a.sendMessage(openWeb ? "OpenWeb" : "Wake up!"))
        return 0;

    ProcessDecorator pd;
    QString plantResult = pd.makeCHFS();
    if(plantResult.isEmpty()==false){
        QMessageBox::critical(nullptr,QObject::tr("错误"),plantResult);
        return -1;
    }

    //日志目录
    logPathRoot = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation)[0];
    QDir().mkpath(logPathRoot);

    // set stylesheet
    QFile file(":/qss/flatblack.css");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream stream(&file);
    a.setStyleSheet(stream.readAll());

    const bool automatic = arguments.contains(QStringLiteral("/auto"))
        || arguments.contains(QStringLiteral("--auto"));
    MainWindow &window = MainWindow::instance();
    if (!automatic && !openWeb && !window.unlock()) return 0;
    window.setVisible(!automatic && !openWeb);
    QObject::connect(&a, SIGNAL(messageReceived(const QString&)),
                     &window, SLOT(onRemoteMessage(const QString&)));
    if (openWeb) QTimer::singleShot(0, &window, SLOT(openSharedPage()));

    return a.exec();
}
