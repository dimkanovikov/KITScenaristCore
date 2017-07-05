#include "ProjectFileWidget.h"
#include "ui_ProjectFileWidget.h"

#include "ProjectUserWidget.h"

#include <3rd_party/Widgets/ElidedLabel/ElidedLabel.h>
#include <3rd_party/Widgets/WAF/Animation/Animation.h>

#include <QLabel>
#include <QMouseEvent>
#include <QTimer>
#include <QVBoxLayout>

using UserInterface::ProjectFileWidget;
using UserInterface::ProjectUserWidget;


ProjectFileWidget::ProjectFileWidget(QWidget *parent) :
    QFrame(parent),
    m_ui(new Ui::ProjectFileWidget)
{
    m_ui->setupUi(this);

    initView();
    initConnections();
    initStylesheet();
}

void ProjectFileWidget::setProjectName(const QString& _projectName)
{
    m_ui->projectName->setText(_projectName);
}

void ProjectFileWidget::setProjectInfo(const QString& _projectInfo)
{
    m_ui->projectInfo->setText(_projectInfo);
}

void ProjectFileWidget::configureOptions(bool _isRemote, bool _isOwner)
{
#ifndef MOBILE_OS
    m_ui->change->setVisible(_isRemote && _isOwner);
    m_ui->remove->setVisible(_isRemote);
    m_ui->share->setVisible(_isRemote && _isOwner);
    m_ui->shareDetails->setVisible(_isRemote);

    m_ui->hide->setVisible(!_isRemote);
#else
    m_ui->change->setVisible(_isOwner);
    m_ui->share->setVisible(_isRemote && _isOwner);
    m_ui->shareDetails->setVisible(_isRemote);

    m_ui->hide->hide();
#endif
}

void ProjectFileWidget::addCollaborator(const QString& _email, const QString& _name, const QString& _role, bool _isOwner)
{
    ProjectUserWidget* user = new ProjectUserWidget(this);
    user->setUserInfo(_email, _name, _role);
    user->setDeletable(_isOwner);
    connect(user, &ProjectUserWidget::removeUserRequested, this, &ProjectFileWidget::removeUserRequested);
    m_ui->usersLayout->addWidget(user);

    m_users.append(user);
}

void ProjectFileWidget::setMouseHover(bool _hover)
{
    //
    // Выделяем если курсор над виджетом
    //
    QString styleSheet;
    if (_hover) {
        styleSheet = "QFrame { background-color: palette(alternate-base); }";
    } else {
        styleSheet = "QFrame { background-color: palette(window); }";
    }
    setStyleSheet(styleSheet);

#ifndef MOBILE_OS
    //
    // Показываем, или скрываем кнопки параметров
    //
    m_ui->optionsPanel->setVisible(_hover);
#endif
}

void ProjectFileWidget::mousePressEvent(QMouseEvent* _event)
{
#ifdef MOBILE_OS
    QColor color = palette().text().color();
    color.setAlphaF(0.05);
    WAF::Animation::circleFillIn(this, _event->globalPos(), color);
#endif
    QWidget::mousePressEvent(_event);
}

void ProjectFileWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    //
    // Выполняем событие, только если курсор так и остался на виджете
    //
    if (rect().contains(_event->pos())) {
        emit clicked();
    }
    QWidget::mouseReleaseEvent(_event);
}

#ifndef MOBILE_OS
void ProjectFileWidget::enterEvent(QEvent* _event)
{
    setMouseHover(true);
    QWidget::enterEvent(_event);
}

void ProjectFileWidget::leaveEvent(QEvent* _event)
{
    setMouseHover(false);
    QWidget::leaveEvent(_event);
}
#endif

void ProjectFileWidget::initView()
{
    setMouseTracking(true);
    setMouseHover(false);

    m_ui->projectInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_ui->projectInfo->setElideMode(Qt::ElideLeft);

    m_ui->change->setIcons(m_ui->change->icon());
    m_ui->remove->setIcons(m_ui->remove->icon());
    m_ui->hide->setIcons(m_ui->hide->icon());
    m_ui->share->setIcons(m_ui->share->icon());
    m_ui->shareDetails->setIcons(m_ui->shareDetails->icon());
    m_ui->openMenu->setIcons(m_ui->openMenu->icon());

#ifdef MOBILE_OS
    m_ui->optionsPanel->hide();
#else
    m_ui->openMenu->hide();
#endif

    m_ui->users->hide();
}

void ProjectFileWidget::initConnections()
{
    connect(m_ui->change, &FlatButton::clicked, this, &ProjectFileWidget::editClicked);
    connect(m_ui->remove, &FlatButton::clicked, this, &ProjectFileWidget::removeClicked);
    connect(m_ui->hide, &FlatButton::clicked, this, &ProjectFileWidget::hideClicked);
    connect(m_ui->share, &FlatButton::clicked, this, &ProjectFileWidget::shareClicked);
    connect(m_ui->shareDetails, &FlatButton::toggled, [=] (bool _toggled) {
        const bool FIX = true;
        if (m_ui->usersLayout->count() > 0) {
            WAF::Animation::slide(m_ui->users, WAF::FromBottomToTop, FIX, !FIX, _toggled);
        }
    });

#ifdef MOBILE_OS
    connect(m_ui->openMenu, &FlatButton::toggled, [=] (bool _show) {
        //
        // Если опции скрываются, скорем и пользователей
        //
        if (_show == false) {
            m_ui->shareDetails->setChecked(_show);
        }
        //
        // Скроем опции, и отожмём выделение проекта
        //
        const bool FIX = true;
        if (_show) {
            m_ui->openMenu->setProperty("projectActionMenu", false);
            setMouseHover(true);
        }
        const int duration = WAF::Animation::slide(m_ui->optionsPanel, WAF::FromRightToLeft, FIX, FIX, _show);
        if (!_show) {
            QTimer::singleShot(duration, [=] {
                m_ui->openMenu->setProperty("projectActionMenu", true);
                setMouseHover(false);
            });
        }
    });
#endif
}

void ProjectFileWidget::initStylesheet()
{
    m_ui->projectInfo->setStyleSheet("color: palette(mid);");
    m_ui->change->setProperty("projectAction", true);
    m_ui->remove->setProperty("projectAction", true);
    m_ui->hide->setProperty("projectAction", true);
    m_ui->share->setProperty("projectAction", true);
    m_ui->shareDetails->setProperty("projectAction", true);
    m_ui->openMenu->setProperty("projectActionMenu", true);
}
