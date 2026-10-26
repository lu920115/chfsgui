#include "advancedconfigwgt.h"

#include "precompile.h"
#include "updatemanager.h"
#include "desktopshortcut.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

AdvancedConfigWgt::AdvancedConfigWgt(Section section, QWidget *parent)
    : QWidget(parent), _section(section)
{
    switch (_section) {
    case PageCustomize: buildPageCustomize(); break;
    case SslHttps: buildSslHttps(); break;
    case ImagePreview: buildImagePreview(); break;
    case FolderDownload: buildFolderDownload(); break;
    case FileRemove: buildFileRemove(); break;
    case SessionTimeout: buildSessionTimeout(); break;
    case PageQrCode: buildPageQrCode(); break;
    case DesktopShortcuts: buildDesktopShortcuts(); break;
    case SoftwareUpdate: buildSoftwareUpdate(); break;
    }
}

void AdvancedConfigWgt::buildPageCustomize()
{
    _titleEdit = new QLineEdit(this);
    _titleEdit->setPlaceholderText(tr("默认：Cute HTTP File Server 局域网文件共享"));
    _titleEdit->setText(g_settings.value("HTML_TITLE", tr("Cute HTTP File Server 局域网文件共享")).toString());
    _titleEdit->setMaximumWidth(360);
    connect(_titleEdit, &QLineEdit::textEdited, this, [](const QString &text) {
        g_settings.setValue("HTML_TITLE", text);
    });

    _noticeEdit = new QPlainTextEdit(this);
    _noticeEdit->setPlaceholderText(tr("默认：手机扫码即可访问共享文件；上传、下载大文件时建议使用浏览器。"));
    _noticeEdit->setPlainText(g_settings.value("HTML_NOTICE", tr("手机扫码即可访问共享文件；上传、下载大文件时建议使用浏览器。")).toString());
    _noticeEdit->setMaximumBlockCount(200);
    connect(_noticeEdit, &QPlainTextEdit::textChanged, this, [this]() {
        g_settings.setValue("HTML_NOTICE", _noticeEdit->toPlainText());
    });

    QFormLayout *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setLabelAlignment(Qt::AlignLeft);
    form->addRow(tr("标题："), _titleEdit);
    form->addRow(tr("公告栏："), _noticeEdit);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 18, 16, 12);
    mainLayout->addLayout(form);
    mainLayout->addStretch(1);
}

void AdvancedConfigWgt::buildSslHttps()
{
    _sslCertEdit = new QLineEdit(this);
    _sslCertEdit->setPlaceholderText(tr("留空则不启用 HTTPS"));
    _sslCertEdit->setText(g_settings.value("SSL_CERT").toString());
    connect(_sslCertEdit, &QLineEdit::textEdited, this, [](const QString &text) {
        g_settings.setValue("SSL_CERT", text);
    });

    _sslKeyEdit = new QLineEdit(this);
    _sslKeyEdit->setPlaceholderText(tr("留空则不启用 HTTPS"));
    _sslKeyEdit->setText(g_settings.value("SSL_KEY").toString());
    connect(_sslKeyEdit, &QLineEdit::textEdited, this, [](const QString &text) {
        g_settings.setValue("SSL_KEY", text);
    });

    QFormLayout *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->addRow(tr("证书："), buildCertificateRow(_sslCertEdit, true));
    form->addRow(tr("密钥："), buildCertificateRow(_sslKeyEdit, false));

    QLabel *tip = new QLabel(
        tr("证书和密钥同时填写后启用 HTTPS；使用 HTTPS 时请按证书配置监听端口。"), this);
    tip->setWordWrap(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 18, 16, 12);
    mainLayout->addLayout(form);
    mainLayout->addWidget(tip);
    mainLayout->addStretch(1);
}

void AdvancedConfigWgt::buildImagePreview()
{
    _imagePreviewCheck = new QCheckBox(tr("启用图片文件的缩略图"), this);
    _imagePreviewCheck->setChecked(g_settings.value("IMAGE_PREVIEW", true).toBool());
    connect(_imagePreviewCheck, &QCheckBox::toggled, this, [this](bool checked) {
        g_settings.setValue("IMAGE_PREVIEW", checked);
        _fastImagePreviewCheck->setEnabled(checked);
    });

    _fastImagePreviewCheck = new QCheckBox(tr("点击图片名时先显示自适应轻量预览"), this);
    _fastImagePreviewCheck->setChecked(g_settings.value("FAST_IMAGE_PREVIEW", true).toBool());
    _fastImagePreviewCheck->setEnabled(_imagePreviewCheck->isChecked());
    connect(_fastImagePreviewCheck, &QCheckBox::toggled, this, [](bool checked) {
        g_settings.setValue("FAST_IMAGE_PREVIEW", checked);
    });

    QLabel *tip = new QLabel(
        tr("预览窗口自适应手机、平板和电脑，先加载 CHFS 电脑端生成的 64×64 轻量图；放大显示不会增加图片细节。只有明确选择原图或下载时才传输完整文件。支持常见图片扩展名；无可用缩略图时会明确提示，原图始终不会被修改。"), this);
    tip->setWordWrap(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 18, 16, 12);
    mainLayout->addWidget(_imagePreviewCheck);
    mainLayout->addWidget(_fastImagePreviewCheck);
    mainLayout->addSpacing(8);
    mainLayout->addWidget(tip);
    mainLayout->addStretch(1);
}

void AdvancedConfigWgt::buildFolderDownload()
{
    _folderEnable = new QRadioButton(tr("无限制"), this);
    _folderDisable = new QRadioButton(tr("禁止所有"), this);
    _folderLeaf = new QRadioButton(tr("仅叶子节点"), this);
    const QString value = g_settings.value("FOLDER_DOWNLOAD", "enable").toString();
    if (value == "disable") _folderDisable->setChecked(true);
    else if (value == "leaf") _folderLeaf->setChecked(true);
    else _folderEnable->setChecked(true);

    connect(_folderEnable, &QRadioButton::clicked, this, []() {
        g_settings.setValue("FOLDER_DOWNLOAD", "enable");
    });
    connect(_folderDisable, &QRadioButton::clicked, this, []() {
        g_settings.setValue("FOLDER_DOWNLOAD", "disable");
    });
    connect(_folderLeaf, &QRadioButton::clicked, this, []() {
        g_settings.setValue("FOLDER_DOWNLOAD", "leaf");
    });

    QHBoxLayout *options = new QHBoxLayout;
    options->setSpacing(18);
    options->addWidget(_folderEnable);
    options->addWidget(_folderDisable);
    options->addWidget(_folderLeaf);
    options->addStretch(1);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 18, 16, 12);
    mainLayout->addLayout(options);
    mainLayout->addStretch(1);
}

void AdvancedConfigWgt::buildFileRemove()
{
    _removeSystem = new QRadioButton(tr("系统回收站（推荐）"), this);
    _removeChfs = new QRadioButton(tr("CHFS 专属回收站"), this);
    _removeReal = new QRadioButton(tr("真实删除（谨慎）"), this);
    const QString value = g_settings.value("FILE_REMOVE", "1").toString();
    if (value == "2") _removeChfs->setChecked(true);
    else if (value == "3") _removeReal->setChecked(true);
    else _removeSystem->setChecked(true);

    connect(_removeSystem, &QRadioButton::clicked, this, []() {
        g_settings.setValue("FILE_REMOVE", "1");
    });
    connect(_removeChfs, &QRadioButton::clicked, this, []() {
        g_settings.setValue("FILE_REMOVE", "2");
    });
    connect(_removeReal, &QRadioButton::clicked, this, []() {
        g_settings.setValue("FILE_REMOVE", "3");
    });

    QHBoxLayout *options = new QHBoxLayout;
    options->setSpacing(18);
    options->addWidget(_removeSystem);
    options->addWidget(_removeChfs);
    options->addWidget(_removeReal);
    options->addStretch(1);

    QLabel *tip = new QLabel(tr("真实删除无法通过回收站恢复，请谨慎选择。"), this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 18, 16, 12);
    mainLayout->addLayout(options);
    mainLayout->addSpacing(8);
    mainLayout->addWidget(tip);
    mainLayout->addStretch(1);
}

void AdvancedConfigWgt::buildSessionTimeout()
{
    _sessionTimeoutSpin = new QSpinBox(this);
    _sessionTimeoutSpin->setRange(1, 9999);
    _sessionTimeoutSpin->setSuffix(tr(" 分钟"));
    _sessionTimeoutSpin->setValue(g_settings.value("SESSION_TIMEOUT", 30).toInt());
    connect(_sessionTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [](int value) {
        g_settings.setValue("SESSION_TIMEOUT", value);
    });

    QHBoxLayout *row = new QHBoxLayout;
    row->addWidget(_sessionTimeoutSpin);
    row->addStretch(1);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 18, 16, 12);
    mainLayout->addLayout(row);
    mainLayout->addStretch(1);
}

void AdvancedConfigWgt::buildPageQrCode()
{
    _webQrCheck = new QCheckBox(tr("在浏览器页面显示二维码"), this);
    _webQrCheck->setChecked(g_settings.value("WEB_QR_ENABLED", true).toBool());
    connect(_webQrCheck, &QCheckBox::toggled, this, [](bool checked) {
        g_settings.setValue("WEB_QR_ENABLED", checked);
    });

    _hoverQrCheck = new QCheckBox(tr("鼠标悬停右侧地址时显示二维码"), this);
    _hoverQrCheck->setChecked(g_settings.value("HOVER_QR_ENABLED", false).toBool());
    connect(_hoverQrCheck, &QCheckBox::toggled, this, [](bool checked) {
        g_settings.setValue("HOVER_QR_ENABLED", checked);
    });

    _wifiPasswordCheck = new QCheckBox(tr("允许在网页二维码面板显示当前 Wi-Fi 密码"), this);
    _wifiPasswordCheck->setChecked(g_settings.value("WIFI_PASSWORD_ENABLED", false).toBool());
    connect(_wifiPasswordCheck, &QCheckBox::toggled, this, [](bool checked) {
        g_settings.setValue("WIFI_PASSWORD_ENABLED", checked);
    });

    QLabel *tip = new QLabel(
        tr("网页二维码默认开启，可拖动并记忆位置。悬浮二维码默认关闭，开启后显示在鼠标左上方。Wi-Fi 密码功能默认关闭；开启后，访问此页面的人都可点击“显示密码”查看。开放网络、有线连接或 Windows 不允许读取时会提示不可用。修改后请重启服务生效。"), this);
    tip->setWordWrap(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 18, 16, 12);
    mainLayout->addWidget(_webQrCheck);
    mainLayout->addWidget(_wifiPasswordCheck);
    mainLayout->addWidget(_hoverQrCheck);
    mainLayout->addSpacing(8);
    mainLayout->addWidget(tip);
    mainLayout->addStretch(1);
}

void AdvancedConfigWgt::buildDesktopShortcuts()
{
    QPushButton *create = new QPushButton(tr("创建桌面快捷方式"), this);
    QLabel *tip = new QLabel(
        tr("将在桌面创建程序图标和“CHFS 共享网页”图标。网页图标会在打开时获取当前服务地址，更换 Wi-Fi 后无需重新创建。"), this);
    tip->setWordWrap(true);

    connect(create, &QPushButton::clicked, this, [this]() {
        QString error;
        if (DesktopShortcut::create(&error)) {
            QMessageBox::information(this, tr("桌面快捷方式"),
                tr("已创建程序图标和“CHFS 共享网页”图标。\n网页图标会在打开时获取当前服务地址，更换 Wi-Fi 后无需重新创建。"));
        } else {
            QMessageBox::warning(this, tr("桌面快捷方式"), error);
        }
    });

    QHBoxLayout *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(create);
    buttonRow->addStretch(1);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 18, 16, 12);
    mainLayout->addLayout(buttonRow);
    mainLayout->addWidget(tip);
    mainLayout->addStretch(1);
}

void AdvancedConfigWgt::buildSoftwareUpdate()
{
    _updateManager = new UpdateManager(this);
    _updateRepositoryLabel = new QLabel(this);
    _updateRepositoryLabel->setWordWrap(true);
    const auto refreshRepository = [this]() {
        _updateRepositoryLabel->setText(
            tr("更新仓库：https://github.com/%1").arg(UpdateManager::repository()));
    };
    refreshRepository();

    QPushButton *check = new QPushButton(tr("检查更新"), this);
    QPushButton *configure = new QPushButton(tr("设置 GitHub 仓库"), this);
    QCheckBox *autoCheck = new QCheckBox(tr("启动时自动检查"), this);
    autoCheck->setChecked(_updateManager->autoCheckEnabled());

    connect(check, &QPushButton::clicked, this, [this]() {
        _updateManager->checkForUpdates(false);
    });
    connect(configure, &QPushButton::clicked, this, [this, refreshRepository]() {
        _updateManager->configureRepository();
        refreshRepository();
    });
    connect(autoCheck, &QCheckBox::toggled, this, [this](bool checked) {
        _updateManager->setAutoCheckEnabled(checked);
    });

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(check);
    buttons->addWidget(configure);
    buttons->addWidget(autoCheck);
    buttons->addStretch(1);

    QLabel *tip = new QLabel(
        tr("默认使用作者指定的公开仓库。发现新版本后需确认下载和安装；安装时共享服务会短暂停止。"), this);
    tip->setWordWrap(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 18, 16, 12);
    mainLayout->addWidget(_updateRepositoryLabel);
    mainLayout->addSpacing(6);
    mainLayout->addLayout(buttons);
    mainLayout->addWidget(tip);
    mainLayout->addStretch(1);
}

QWidget *AdvancedConfigWgt::buildCertificateRow(QLineEdit *edit, bool certificate)
{
    QWidget *row = new QWidget(this);
    QPushButton *browse = new QPushButton(tr("浏览"), row);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(edit, 1);
    layout->addWidget(browse);
    connect(browse, &QPushButton::clicked, this, [this, edit, certificate]() {
        QString file = QFileDialog::getOpenFileName(
            this,
            certificate ? tr("选择 SSL 证书") : tr("选择 SSL 私钥"),
            QString(),
            tr("证书文件 (*.crt *.pem *.cer *.key);;所有文件 (*)")
        );
        if (!file.isEmpty()) {
            edit->setText(file);
            g_settings.setValue(certificate ? "SSL_CERT" : "SSL_KEY", file);
        }
    });
    return row;
}

void AdvancedConfigWgt::setRunning(bool running)
{
    if (_section != DesktopShortcuts && _section != SoftwareUpdate) setDisabled(running);
}
