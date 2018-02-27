#include "ProjectFileWidget.h"
#include "ui_ProjectFileWidget.h"

#include "ProjectUserWidget.h"

#include <3rd_party/Widgets/ElidedLabel/ElidedLabel.h>
#include <3rd_party/Widgets/WAF/Animation/Animation.h>

#include <QDateTime>
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
    //
    // Настриваем выравнивание, чтобы все имена были выровнены по одному краю,
    // вне зависимости от языка на котором они написаны
    //
    if ((isRightToLeft() && !_projectName.isRightToLeft())
        || (!isRightToLeft() && _projectName.isRightToLeft())) {
        m_ui->projectName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
}

void ProjectFileWidget::setProjectInfo(const QString& _projectInfo)
{
    m_ui->projectInfo->setText(_projectInfo);
    //
    // Настриваем выравнивание, чтобы все строки с дополнительной информацией были выровнены
    // по одному краю, вне зависимости от языка на котором они написаны
    //
    if ((isRightToLeft() && !_projectInfo.isRightToLeft())
        || (!isRightToLeft() && _projectInfo.isRightToLeft())) {
        m_ui->projectInfo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
}

void ProjectFileWidget::configureOptions(bool _isRemote, bool _isOwner)
{
#ifndef MOBILE_OS
    m_ui->change->setVisible(_isRemote && _isOwner);
    m_ui->remove->setVisible(_isRemote);
    m_ui->moveToCloud->setVisible(!_isRemote);
    m_ui->share->setVisible(_isRemote && _isOwner);
    m_ui->shareDetails->setVisible(_isRemote);

    m_ui->hide->setVisible(!_isRemote);
#else
    m_ui->change->setVisible(_isOwner);
    m_ui->moveToCloud->setVisible(!_isRemote);
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

void ProjectFileWidget::setMenuVisible(bool _isVisible)
{
    m_ui->openMenu->setVisible(_isVisible);
}

void ProjectFileWidget::mousePressEvent(QMouseEvent* _event)
{
    QColor color = palette().text().color();
    color.setAlphaF(0.05);
    m_clickedAt = QDateTime::currentMSecsSinceEpoch();
    WAF::Animation::circleFillIn(this, _event->globalPos(), color);

    QWidget::mousePressEvent(_event);
}

void ProjectFileWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    //
    // Выполняем событие, только если курсор так и остался на виджете
    //
    if (rect().contains(_event->pos())) {
        const int delay = qMax(m_clickedAt + 400 - QDateTime::currentMSecsSinceEpoch(), quint64(0));
        QTimer::singleShot(delay, this, &ProjectFileWidget::clicked);
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
    m_ui->moveToCloud->setIcons(m_ui->moveToCloud->icon());
    m_ui->share->setIcons(m_ui->share->icon());
    m_ui->shareDetails->setIcons(m_ui->shareDetails->icon());
    m_ui->openMenu->setIcons(m_ui->openMenu->icon());

#ifdef MOBILE_OS
    m_ui->optionsPanel->hide();
    m_ui->change->setToolTip(QString());
    m_ui->remove->setToolTip(QString());
    m_ui->hide->setToolTip(QString());
    m_ui->moveToCloud->setToolTip(QString());
    m_ui->share->setToolTip(QString());
    m_ui->shareDetails->setToolTip(QString());
    m_ui->openMenu->setToolTip(QString());
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
    connect(m_ui->moveToCloud, &FlatButton::clicked, this, &ProjectFileWidget::moveToCloudClicked);
    connect(m_ui->share, &FlatButton::clicked, this, &ProjectFileWidget::shareClicked);
    connect(m_ui->shareDetails, &FlatButton::toggled, [=] (bool _toggled) {
        const bool FIX = true;
        if (m_ui->usersLayout->count() > 0) {
            WAF::Animation::slide(m_ui->users, WAF::FromBottomToTop, FIX, FIX, _toggled);
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
    setProperty("projectFrame", true);
    m_ui->frame->setProperty("projectTextFrame", true);
    m_ui->projectName->setProperty("projectName", true);
    m_ui->projectInfo->setProperty("projectInfo", true);
    m_ui->change->setProperty("projectAction", true);
    m_ui->remove->setProperty("projectAction", true);
    m_ui->hide->setProperty("projectAction", true);
    m_ui->moveToCloud->setProperty("projectAction", true);
    m_ui->share->setProperty("projectAction", true);
    m_ui->shareDetails->setProperty("projectAction", true);
    m_ui->openMenu->setProperty("projectActionMenu", true);
}
