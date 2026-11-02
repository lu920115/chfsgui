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
    void relock();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    QWidget *createSection(const QString &title, QWidget *content);
    void addAdvancedSection(const QString &name, AdvancedConfigWgt::Section section,
                            bool protect = true);
    void addNavigationSection(const QString &name, QWidget *content);
    void protectContent(QWidget *content);
    void scrollToSection(int row);

    bool _unlocked = false;

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
