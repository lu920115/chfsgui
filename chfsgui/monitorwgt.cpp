#include "monitorwgt.h"
#include "precompile.h"
#include <QTextBrowser>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QMenu>
#include <QHostAddress>
#include <QUrl>
#include <QVector>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

namespace {

QString defaultRouteIpv4()
{
#ifdef Q_OS_WIN
    DWORD interfaceIndex = 0;
    sockaddr_in destination = {};
    destination.sin_family = AF_INET;
    destination.sin_addr.s_addr = htonl(0xC0000201); // 192.0.2.1，仅用于查询路由，不发送数据。
    if (GetBestInterfaceEx(reinterpret_cast<sockaddr *>(&destination), &interfaceIndex) != NO_ERROR) {
        return QString();
    }

    ULONG size = 16 * 1024;
    QVector<char> storage(static_cast<int>(size));
    DWORD result = ERROR_BUFFER_OVERFLOW;
    for (int attempt = 0; attempt < 3 && result == ERROR_BUFFER_OVERFLOW; ++attempt) {
        storage.resize(static_cast<int>(size));
        result = GetAdaptersAddresses(AF_INET,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
            nullptr, reinterpret_cast<PIP_ADAPTER_ADDRESSES>(storage.data()), &size);
    }
    if (result != NO_ERROR) return QString();

    for (PIP_ADAPTER_ADDRESSES adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(storage.data());
         adapter; adapter = adapter->Next) {
        if (adapter->IfIndex != interfaceIndex || adapter->OperStatus != IfOperStatusUp) continue;
        for (PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress;
             address; address = address->Next) {
            if (!address->Address.lpSockaddr || address->Address.lpSockaddr->sa_family != AF_INET) continue;
            const sockaddr_in *ipv4 = reinterpret_cast<const sockaddr_in *>(address->Address.lpSockaddr);
            char text[INET_ADDRSTRLEN] = {};
            if (InetNtopA(AF_INET, const_cast<IN_ADDR *>(&ipv4->sin_addr), text, sizeof(text))) {
                return QString::fromLatin1(text);
            }
        }
    }
#endif
    return QString();
}

QString selectWebsiteUrl(const QStringList &candidates)
{
    const QString preferredHost = defaultRouteIpv4();
    if (!preferredHost.isEmpty()) {
        for (const QString &candidate : candidates) {
            if (QUrl(candidate).host().compare(preferredHost, Qt::CaseInsensitive) == 0) {
                return candidate;
            }
        }
    }

    QString best;
    int bestScore = -1;
    for (const QString &candidate : candidates) {
        const QString host = QUrl(candidate).host();
        const QHostAddress address(host);
        if (host.isEmpty() || address.isLoopback() || host == QStringLiteral("0.0.0.0")) continue;
        int score = 10;
        const QStringList parts = host.split('.');
        if (parts.size() == 4) {
            const int first = parts[0].toInt();
            const int second = parts[1].toInt();
            if (first == 192 && second == 168) score = 100;
            else if (first == 10) score = 90;
            else if (first == 172 && second >= 16 && second <= 31) score = 50;
            if (host == QStringLiteral("172.26.0.1")) score = 1;
        }
        if (score > bestScore) {
            bestScore = score;
            best = candidate;
        }
    }
    return best;
}

}

bool hoverQrEnabled()
{
    return g_settings.value("HOVER_QR_ENABLED", false).toBool();
}

LogWidget::LogWidget(QWidget *parent) : QWidget(parent)
{
    logList = new QListWidget(this);

    QLabel* logBtn = new QLabel("<a href=#><span style=\"color:silver;\">操作日志</span></a>:",this);
    logBtn->setToolTip("点击打开操作日志目录");
    connect(logBtn,&QLabel::linkActivated,[=](){
        extern QString logPathRoot;
        QDesktopServices::openUrl(QUrl::fromLocalFile(logPathRoot));
    });

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(2);
    mainLayout->addWidget( logBtn );
    mainLayout->addWidget( logList );

    createActions();
}

void LogWidget::contextMenuEvent(QContextMenuEvent *event)
{
    menu->exec(event->globalPos());
}

void LogWidget::createActions()
{
    menu = new QMenu;
    menu->addAction(tr("清空显示"),this,[=](){
        logList->clear();
    });
}

void LogWidget::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

MonitorWgt::MonitorWgt(QWidget *parent) : QWidget(parent)
{
    MyTextBrowser* editorStatus = new MyTextBrowser(this);
    editorStatus->setOpenExternalLinks(true);
    editorStatus->setOpenLinks(true);
    editorStatus->setPlaceholderText(tr("服务未运行"));
    editorStatus->setFixedHeight(100);

    LogWidget* editorLog = new LogWidget(this);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(15);
    mainLayout->addWidget( editorStatus );
    mainLayout->addWidget( editorLog,1 );

    //
    // connections
    //

    connect(&g_chfsProcess,&QProcess::readyReadStandardOutput,this,[=](){
        QString msg = g_chfsProcess.readAllStandardOutput();

        QRegExp urlReg(R"(https?://[0-9+|.:]+[0-9](?:/webdav)?)");
        for(QString line: msg.split("\n")){
            if( line.isEmpty() == false ){
                //TODO 使用正则进行校验
                if( line.length() > 50 && line[4]=='-'  && line[7]=='-'  && line[10]==' ' ){
                    if( editorLog->logList->count()>1000 ){
                        editorLog->logList->clear();
                    }
                    QListWidgetItem* item = new QListWidgetItem(line);
                    if( line.contains(") removed") ){
                        item->setForeground( Qt::red );
                    }else if( line.contains(") rename") || line.contains(") uploaded") || line.contains(") create")){
                        item->setForeground( Qt::darkYellow );
                    }
                    editorLog->logList->addItem( item );
                    editorLog->logList->scrollToBottom();
                }else{
                    urlReg.indexIn(line);
                    QStringList urls = urlReg.capturedTexts();
                    for(auto url: urls){
                        if( url.isEmpty() == false ){
                             const QString label = line.contains("webdav", Qt::CaseInsensitive)
                                 ? QStringLiteral("Webdav") : QStringLiteral("Website");
                             QString newTextItem = QString("<pre>&nbsp;&nbsp;%1&nbsp;:&nbsp;<a href=\"%2\" style=\"color:green\">%3</a></pre>")
                                 .arg(label).arg(url).arg(url);
                             editorStatus->append( newTextItem );
                             if (label == QStringLiteral("Website") && !_websiteCandidates.contains(url)) {
                                 _websiteCandidates.append(url);
                             }
                        }
                    }
                }
            }
        }
        const QString selected = selectWebsiteUrl(_websiteCandidates);
        if (!selected.isEmpty() && selected != _selectedWebsiteUrl) {
            _selectedWebsiteUrl = selected;
            g_settings.setValue("LAST_WEBSITE_URL", selected);
            emit sigWebsiteUrl(selected);
        }
    });

    connect(&g_chfsProcess,&QProcess::readyReadStandardError,this,[=](){
        editorStatus->append( QString("<font color=\"red\">") + g_chfsProcess.readAllStandardError() + QString("</font>") );
    });

    connect(&g_chfsProcess,&QProcess::started,this,[=](){
        _websiteCandidates.clear();
        _selectedWebsiteUrl.clear();
        editorStatus->clear();
        editorLog->logList->clear();
    });

    connect(&g_chfsProcess,static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),[=](int , QProcess::ExitStatus exitStatus){
        if( exitStatus == QProcess::CrashExit ){
            editorStatus->clear();
            editorLog->logList->clear();
        }
    });
}

void MonitorWgt::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
