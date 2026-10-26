#ifndef CONFIGWGT_H
#define CONFIGWGT_H

#include <QList>
#include "uicomponents/configwgt/advancedconfigwgt.h"

class CfgSharedpathWgt;
class CfgAllowip;
class CfgRules;
class CfgPortWgt;
class AdvancedConfigWgt;
class QListWidget;
class QScrollArea;
class QVBoxLayout;

class ConfigWgt : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigWgt(QWidget *parent = 0);
    void paintEvent(QPaintEvent *) override;

    void createComponents();
    void onEditorMode();
    void onRunningMode();
private:
    QWidget *createSection(const QString &title, QWidget *content);
    void addAdvancedSection(const QString &name, AdvancedConfigWgt::Section section);
    void addNavigationSection(const QString &name, QWidget *content);
    void scrollToSection(int row);

    CfgSharedpathWgt* _sharedpathWgt;
    CfgAllowip* _allowWgt;
    CfgRules* _rulesWgt;
    CfgPortWgt* _portWgt;
    QListWidget *_navigation;
    QScrollArea *_scrollArea;
    QWidget *_pageContent;
    QVBoxLayout *_pageLayout;
    QList<QWidget*> _sections;
    QList<AdvancedConfigWgt*> _advancedWgts;
};

#endif // CONFIGWGT_H
