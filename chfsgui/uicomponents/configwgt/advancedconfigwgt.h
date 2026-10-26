#ifndef ADVANCEDCONFIGWGT_H
#define ADVANCEDCONFIGWGT_H

#include <QWidget>

class QCheckBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QRadioButton;
class QLabel;
class UpdateManager;

class AdvancedConfigWgt : public QWidget
{
    Q_OBJECT
public:
    enum Section {
        PageCustomize,
        SslHttps,
        ImagePreview,
        FolderDownload,
        FileRemove,
        SessionTimeout,
        PageQrCode,
        SoftwareUpdate
    };

    explicit AdvancedConfigWgt(Section section, QWidget *parent = nullptr);
    void setRunning(bool running);

private:
    QWidget *buildCertificateRow(QLineEdit *edit, bool certificate);
    void buildPageCustomize();
    void buildSslHttps();
    void buildImagePreview();
    void buildFolderDownload();
    void buildFileRemove();
    void buildSessionTimeout();
    void buildPageQrCode();
    void buildSoftwareUpdate();

    Section _section;
    QLineEdit *_titleEdit = nullptr;
    QPlainTextEdit *_noticeEdit = nullptr;
    QCheckBox *_imagePreviewCheck = nullptr;
    QCheckBox *_fastImagePreviewCheck = nullptr;
    QCheckBox *_webQrCheck = nullptr;
    QCheckBox *_hoverQrCheck = nullptr;
    QCheckBox *_wifiPasswordCheck = nullptr;
    QRadioButton *_folderEnable = nullptr;
    QRadioButton *_folderLeaf = nullptr;
    QRadioButton *_folderDisable = nullptr;
    QRadioButton *_removeSystem = nullptr;
    QRadioButton *_removeChfs = nullptr;
    QRadioButton *_removeReal = nullptr;
    QSpinBox *_sessionTimeoutSpin = nullptr;
    QLineEdit *_sslCertEdit = nullptr;
    QLineEdit *_sslKeyEdit = nullptr;
    UpdateManager *_updateManager = nullptr;
    QLabel *_updateRepositoryLabel = nullptr;
};

#endif // ADVANCEDCONFIGWGT_H
