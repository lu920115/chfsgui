#include "aboutdlg.h"
#include "ui_aboutdlg.h"
#include "precompile.h"


QString AboutDlg::getCHFSver()
{
    QProcess chfs;
    chfs.start(g_chfsfile, QStringList() << "--version");
    chfs.waitForFinished();

    return QString(chfs.readAllStandardError()).remove("\n");
}

AboutDlg::AboutDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDlg)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);

    const QString appTitle = parentWidget()
        ? parentWidget()->window()->windowTitle()
        : QStringLiteral("Cute HTTP File Server 3.1 二维码增强版");
    ui->_lbTitle->setText(QString("<h2>%1</h2>").arg(appTitle.toHtmlEscaped()));
}

AboutDlg::~AboutDlg()
{
    delete ui;
}

void AboutDlg::on_pushButton_clicked()
{
    close();
}
