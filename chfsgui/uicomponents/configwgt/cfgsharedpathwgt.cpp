#include "cfgsharedpathwgt.h"
#include "precompile.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QListWidget>
#include <QDir>

CfgSharedpathWgt::CfgSharedpathWgt(QWidget *parent) : CfgBase(parent)
{
    _pathList = new QListWidget(this);
    _pathList->setSelectionMode(QAbstractItemView::SingleSelection);
    _pathList->setAlternatingRowColors(false);

    _btnAdd = new QPushButton(tr("添加"), this);
    _btnRemove = new QPushButton(tr("删除"), this);
    _btnOpen = new QPushButton(tr("打开"), this);

    QVBoxLayout *buttons = new QVBoxLayout;
    buttons->setMargin(0);
    buttons->setSpacing(6);
    buttons->addWidget(_btnAdd);
    buttons->addWidget(_btnRemove);
    buttons->addStretch(1);
    buttons->addWidget(_btnOpen);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->addWidget(_pathList, 1);
    mainLayout->addLayout(buttons);

    QStringList paths = g_settings.value(PARAM_PATH).toString().split('|', Qt::SkipEmptyParts);
    if (paths.isEmpty()) paths.append(QApplication::applicationDirPath());
    for (const QString &path : paths) addPath(path);
    savePaths();
    if (_pathList->count()) _pathList->setCurrentRow(0);

    connect(_btnAdd, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(
            this, tr("添加共享目录"), QString(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            addPath(dir);
            savePaths();
            _pathList->setCurrentRow(_pathList->count() - 1);
        }
    });
    connect(_btnRemove, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = _pathList->currentItem();
        if (!item) return;
        if (_pathList->count() == 1) {
            QMessageBox::information(this, tr("共享目录"), tr("请至少保留一个共享目录。"));
            return;
        }
        delete _pathList->takeItem(_pathList->row(item));
        savePaths();
    });
    connect(_btnOpen, &QPushButton::clicked, this, &CfgSharedpathWgt::onPathChanging);
    connect(_pathList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
        onPathChanging();
    });
}

void CfgSharedpathWgt::addPath(const QString &path)
{
    const QString clean = QDir::cleanPath(path);
    for (int i = 0; i < _pathList->count(); ++i) {
        if (_pathList->item(i)->text().compare(clean, Qt::CaseInsensitive) == 0) return;
    }
    QListWidgetItem *item = new QListWidgetItem(QIcon(":/res/image/dir.svg"), clean);
    item->setToolTip(clean);
    _pathList->addItem(item);
}

void CfgSharedpathWgt::savePaths()
{
    QStringList paths;
    for (int i = 0; i < _pathList->count(); ++i) paths.append(_pathList->item(i)->text());
    g_settings.setValue(PARAM_PATH, paths.join('|'));
}

QString CfgSharedpathWgt::selectedPath() const
{
    QListWidgetItem *item = _pathList->currentItem();
    return item ? item->text() : QString();
}

void CfgSharedpathWgt::onEditorMode()
{
    _btnAdd->setVisible(true);
    _btnRemove->setVisible(true);
    _pathList->setEnabled(true);
}

void CfgSharedpathWgt::onRunningMode()
{
    _btnAdd->setVisible(false);
    _btnRemove->setVisible(false);
    _pathList->setEnabled(true);
    _pathList->clearFocus();
}

void CfgSharedpathWgt::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void CfgSharedpathWgt::onPathChanging()
{
    const QString path = selectedPath();
    if (!path.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}
