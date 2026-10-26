#include "mainwindow.h"
#include "precompile.h"
#include "qrconfig.h"
#include "appprotection.h"
#include "appversion.h"
#include <QDesktopWidget>
#include <QApplication>
#include "toolbar.h"
#include "configwgt.h"
#include "monitorwgt.h"
#include <QMenu>
#include <QDir>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QTimer>
#include <QUrl>

QProcess g_chfsProcess;
SomeActions someActions;
QSettings g_settings(APPDOMAIN, APPKEY);


void setAppAutorun(bool isAuto)
{
    if( isAuto ){
        QSettings(REG_RUN,QSettings::NativeFormat).setValue(APPKEY,QString("\"%1\" /auto").arg(
                                                                   QDir::toNativeSeparators(QApplication::applicationFilePath())
                                                                )
                                                            );
    }else{
        QSettings(REG_RUN,QSettings::NativeFormat).remove(APPKEY);
    }
}

#define SETWINDOWSIZE()   setFixedSize(900,610)
MainWindow::MainWindow(QWidget *parent)
    : QFrame(parent)
{
    setWindowIcon(QIcon(":/APP_LOGO"));
    setWindowTitle(APP_TITLE);
    SETWINDOWSIZE(); //阻止界面初始化时让主窗口身材变形


    //
    // create contents
    //

    createContents();
    createActions();

    //
    // connections
    //

    connect(ui.toolbarWgt,&Toolbar::sigEditorMode,this,&MainWindow::onEditorMode);
    connect(ui.toolbarWgt,&Toolbar::sigRunningMode,this,&MainWindow::onRunningMode);
    connect(ui.toolbarWgt,&Toolbar::sigFireLaunch,this,&MainWindow::fireLaunch);
    connect(&g_chfsProcess,&QProcess::started,this,[=](){
        _websiteUrl.clear();
    });
    connect(ui.monitorWgt,&MonitorWgt::sigWebsiteUrl,this,[=](const QString &url){
        _websiteUrl = url;
        if (_openWebWhenReady) {
            _openWebWhenReady = false;
            QDesktopServices::openUrl(QUrl(url));
        }
    });
    connect(ui.toolbarWgt,&Toolbar::sigQuit,this,&MainWindow::close);
    connect(ui.toolbarWgt,&Toolbar::sigAutorunSwitching,this,[=](bool isAuto){        
        setAppAutorun(isAuto);
        QSettings(APPDOMAIN, QString(APPKEY)+"/run").setValue(AUTORUNFORBIDDEN,!isAuto);
    });


    //
    // 初始化
    //

    onEditorMode();

    //是否启用开机自运行
    setAppAutorun( !QSettings(APPDOMAIN, QString(APPKEY)+"/run").value(AUTORUNFORBIDDEN).toBool() );


    //
    // 设置窗口尺寸
    //

    SETWINDOWSIZE();
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

MainWindow::~MainWindow()
{

}

void MainWindow::closeEvent(QCloseEvent * event)
{
    if( sender() ){
        event->accept();
        g_chfsProcess.kill();
        g_chfsProcess.waitForFinished();
    }else{
        event->ignore();
        hide();
    }
}

void MainWindow::createActions()
{
    QMenu* trayMenu = new QMenu;
    trayMenu->addAction(tr("显示主窗口"),this,SLOT(onShowWindow()));
    trayMenu->addSeparator();
    trayMenu->addAction(someActions.autoRunAction);
    trayMenu->addSeparator();
    trayMenu->addAction(tr("退出"),this,SLOT(close()));

    ui.trayicon = new QSystemTrayIcon(QIcon(":/APP_LOGO"),this);
    connect(ui.trayicon,&QSystemTrayIcon::activated,this,[=](QSystemTrayIcon::ActivationReason  r){
        if( r == QSystemTrayIcon::DoubleClick )
            onShowWindow();
    });
    ui.trayicon->setContextMenu(trayMenu);
    ui.trayicon->show();

    void (QProcess::* signalFinished)(int,QProcess::ExitStatus) = &QProcess::finished;
    connect(&g_chfsProcess,signalFinished,this,[=](){
        if( g_chfsProcess.exitStatus() == QProcess::CrashExit){ //手动KILL进程
        }else{
            ui.trayicon->setIcon(QIcon(":/res/image/tray_error.png"));
        }
    });
}

bool MainWindow::unlock()
{
    if (!_unlocked) {
        _unlocked = AppProtection::unlock(this);
    }
    return _unlocked;
}

void MainWindow::onShowWindow()
{
    if (!unlock()) return;
    showNormal();
    activateWindow();
    raise();
}

void MainWindow::onRemoteMessage(const QString &message)
{
    if (message == QStringLiteral("OpenWeb")) {
        openSharedPage();
    } else {
        onShowWindow();
    }
}

void MainWindow::openSharedPage()
{
    onShowWindow();
    if (!_unlocked) return;

    if (g_chfsProcess.state() != QProcess::NotRunning && !_websiteUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(_websiteUrl));
        return;
    }

    _openWebWhenReady = true;
    if (g_chfsProcess.state() == QProcess::NotRunning) fireLaunch();
    QTimer::singleShot(15000, this, [=]() {
        if (!_openWebWhenReady) return;
        _openWebWhenReady = false;
        QMessageBox::warning(this, tr("打开共享网页"),
            tr("尚未获取到服务地址。请检查端口、防火墙和运行状态后重试。"));
    });
}

void MainWindow::createContents()
{
    ui.toolbarWgt = new Toolbar(this);
    ui.configWgt = new ConfigWgt(this);
    ui.monitorWgt = new MonitorWgt(this);

    ui.contentLayout = new QHBoxLayout;
    ui.contentLayout->setContentsMargins(10,10,10,10);
    ui.contentLayout->setSpacing(12);
    ui.contentLayout->addWidget(ui.configWgt,4);
    ui.contentLayout->addWidget(ui.monitorWgt,3);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(ui.toolbarWgt);
    mainLayout->addLayout(ui.contentLayout);
}


void MainWindow::fireLaunch()
{
    QStringList arguments;
    QString qrConfigPath;
    QString qrConfigError;
    if (createQrConfig(qrConfigPath, qrConfigError)) {
        arguments << QString("--file=%1").arg(qrConfigPath);
    } else {
        arguments << QString("--path=%1").arg(g_settings.value(PARAM_PATH).toString());
        arguments << QString("--port=%1").arg(g_settings.value(PARAM_PORT).toString());
        QMessageBox::warning(
            this,
            tr("二维码配置"),
            qrConfigError + tr("\n将不启用二维码，并使用基础设置启动。")
        );
    }

    g_chfsProcess.start(g_chfsfile,arguments);
}

void MainWindow::onEditorMode()
{
    ui.configWgt->onEditorMode();
    ui.trayicon->setIcon(QIcon(":/APP_LOGO"));
}

void MainWindow::onRunningMode()
{
    ui.configWgt->onRunningMode();
    ui.trayicon->setIcon(QIcon(":/res/image/tray_running.png"));    
}

