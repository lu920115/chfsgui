#include "configwgt.h"
#include "precompile.h"
#include "appprotection.h"
#include "uicomponents/configwgt/cfgsharedpathwgt.h"
#include "uicomponents/configwgt/cfgallowip.h"
#include "uicomponents/configwgt/cfgrules.h"
#include "uicomponents/configwgt/cfgportwgt.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>

ConfigWgt::ConfigWgt(QWidget *parent) : QWidget(parent)
{
    createComponents();
    setProperty("form", "true");
}

void ConfigWgt::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ConfigWgt::onEditorMode()
{
    CfgBase* ws[]{_sharedpathWgt, _allowWgt, _rulesWgt, _portWgt};
    for (auto i : ws) i->onEditorMode();
    for (auto w : _advancedWgts) w->setRunning(false);
}

void ConfigWgt::onRunningMode()
{
    CfgBase* ws[]{_sharedpathWgt, _allowWgt, _rulesWgt, _portWgt};
    for (auto i : ws) i->onRunningMode();
    for (auto w : _advancedWgts) w->setRunning(true);
}

QWidget *ConfigWgt::createSection(const QString &title, QWidget *content)
{
    QWidget *section = new QWidget(_pageContent);
    section->setObjectName("chfsConfigSection");
    QLabel *heading = new QLabel(title, section);
    heading->setObjectName("chfsConfigSectionTitle");
    heading->setStyleSheet(
        "QLabel#chfsConfigSectionTitle{font-size:15px;font-weight:600;"
        "padding:3px 0 8px 0;color:#E8E8E8;}"
    );
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(14, 10, 16, 14);
    layout->setSpacing(4);
    layout->addWidget(heading);
    layout->addWidget(content);
    return section;
}

void ConfigWgt::addNavigationSection(const QString &name, QWidget *content)
{
    QWidget *section = createSection(name, content);
    _navigation->addItem(name);
    _sections.append(section);
    _pageLayout->addWidget(section);
}

void ConfigWgt::addAdvancedSection(const QString &name, AdvancedConfigWgt::Section section,
                                   bool protect)
{
    AdvancedConfigWgt *content = new AdvancedConfigWgt(section, _pageContent);
    _advancedWgts.append(content);
    addNavigationSection(name, content);
    if (protect) protectContent(content);
}

void ConfigWgt::protectContent(QWidget *content)
{
    const auto widgets = content->findChildren<QWidget *>();
    for (QWidget *widget : widgets) {
        if (qobject_cast<QAbstractButton *>(widget) ||
            qobject_cast<QAbstractSpinBox *>(widget) ||
            qobject_cast<QAbstractItemView *>(widget) ||
            qobject_cast<QComboBox *>(widget) ||
            qobject_cast<QLineEdit *>(widget) ||
            qobject_cast<QPlainTextEdit *>(widget)) {
            widget->installEventFilter(this);
        }
    }
}

bool ConfigWgt::eventFilter(QObject *watched, QEvent *event)
{
    const QEvent::Type type = event->type();
    const bool input = type == QEvent::MouseButtonPress ||
        type == QEvent::MouseButtonDblClick || type == QEvent::KeyPress ||
        type == QEvent::Wheel || type == QEvent::ContextMenu || type == QEvent::Drop;
    if (!input) return QWidget::eventFilter(watched, event);

    // Allow ordinary list selection for viewing, but protect editing actions.
    if (qobject_cast<QAbstractItemView *>(watched) &&
        type != QEvent::KeyPress && type != QEvent::MouseButtonDblClick &&
        type != QEvent::ContextMenu && type != QEvent::Drop) {
        return false;
    }
    if (!AppProtection::isEnabled()) {
        _unlocked = false;
        return false;
    }
    if (!_unlocked) {
        if (!AppProtection::unlock(this)) return true;
        _unlocked = true;
    }
    return false;
}

void ConfigWgt::relock()
{
    _unlocked = false;
}

void ConfigWgt::scrollToSection(int row)
{
    if (row < 0 || row >= _sections.count()) return;
    QWidget *target = _sections.at(row);
    QTimer::singleShot(0, this, [this, target]() {
        _scrollArea->verticalScrollBar()->setValue(target->y());
    });
}

void ConfigWgt::createComponents()
{
    _sharedpathWgt = new CfgSharedpathWgt(this);
    _allowWgt = new CfgAllowip(this);
    _rulesWgt = new CfgRules(this);
    _portWgt = new CfgPortWgt(this);

    _navigation = new QListWidget(this);
    _navigation->setObjectName("chfsOfficialNavigation");
    _navigation->setFixedWidth(108);
    _navigation->setSpacing(2);
    _navigation->setFrameShape(QFrame::NoFrame);
    _navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _navigation->setFocusPolicy(Qt::NoFocus);
    _navigation->setStyleSheet(
        "QListWidget#chfsOfficialNavigation{background:transparent;border:0;outline:0;padding:4px 3px;}"
        "QListWidget#chfsOfficialNavigation::item{height:30px;padding-left:8px;border:0;border-radius:2px;}"
        "QListWidget#chfsOfficialNavigation::item:selected{background:#3D6E75;color:#FFFFFF;}"
        "QListWidget#chfsOfficialNavigation::item:hover:!selected{background:#454B4D;}"
    );

    _scrollArea = new QScrollArea(this);
    _scrollArea->setWidgetResizable(true);
    _scrollArea->setFrameShape(QFrame::NoFrame);
    _scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _scrollArea->setAlignment(Qt::AlignTop);

    _pageContent = new QWidget(_scrollArea);
    _pageLayout = new QVBoxLayout(_pageContent);
    _pageLayout->setContentsMargins(0, 0, 4, 0);
    _pageLayout->setSpacing(4);

    addNavigationSection(tr("共享目录"), _sharedpathWgt);
    protectContent(_sharedpathWgt);
    addNavigationSection(tr("监听端口"), _portWgt);
    protectContent(_portWgt);
    addNavigationSection(tr("账户控制"), _rulesWgt);
    protectContent(_rulesWgt);
    addAdvancedSection(tr("页面定制"), AdvancedConfigWgt::PageCustomize);
    addNavigationSection(tr("地址过滤"), _allowWgt);
    protectContent(_allowWgt);
    addAdvancedSection(tr("SSL/https"), AdvancedConfigWgt::SslHttps);
    addAdvancedSection(tr("图片预览"), AdvancedConfigWgt::ImagePreview);
    addAdvancedSection(tr("目录下载"), AdvancedConfigWgt::FolderDownload);
    addAdvancedSection(tr("文件删除"), AdvancedConfigWgt::FileRemove);
    addAdvancedSection(tr("会话时长"), AdvancedConfigWgt::SessionTimeout);
    addAdvancedSection(tr("页面二维码"), AdvancedConfigWgt::PageQrCode);
    addAdvancedSection(tr("快捷方式"), AdvancedConfigWgt::DesktopShortcuts, false);
    addAdvancedSection(tr("更新"), AdvancedConfigWgt::SoftwareUpdate, false);

    _pageLayout->addStretch(1);
    _scrollArea->setWidget(_pageContent);

    connect(_navigation, &QListWidget::currentRowChanged,
            this, &ConfigWgt::scrollToSection);
    _navigation->setCurrentRow(0);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(8);
    mainLayout->addWidget(_navigation);
    mainLayout->addWidget(_scrollArea, 1);
}

