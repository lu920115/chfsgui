#ifndef CFGSHAREDPATHWGT_H
#define CFGSHAREDPATHWGT_H
#include "cfgbase.h"

class QListWidget;
class QPushButton;

class CfgSharedpathWgt : public CfgBase
{
    Q_OBJECT
public:
    explicit CfgSharedpathWgt(QWidget *parent = 0);
    void paintEvent(QPaintEvent *) override;

    virtual void onEditorMode();
    virtual void onRunningMode();
private:
    void addPath(const QString &path);
    void savePaths();
    QString selectedPath() const;

    QListWidget *_pathList;
    QPushButton *_btnAdd;
    QPushButton *_btnRemove;
    QPushButton *_btnOpen;
signals:

public slots:
    void onPathChanging();
};

#endif // CFGSHAREDPATHWGT_H
