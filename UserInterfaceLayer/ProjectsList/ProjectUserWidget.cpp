#include "ProjectUserWidget.h"
#include "ui_ProjectUserWidget.h"

#include <3rd_party/Helpers/TextUtils.h>
#include <3rd_party/Helpers/ImageHelper.h>

using UserInterface::ProjectUserWidget;


ProjectUserWidget::ProjectUserWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::ProjectUserWidget)
{
    m_ui->setupUi(this);

    initView();
    initConnections();
}

ProjectUserWidget::~ProjectUserWidget()
{
    delete m_ui;
}

void ProjectUserWidget::setUserInfo(const QString& _email, const QString& _name, const QString& _role)
{
    m_ui->userName->setText(QString("%1 %2").arg(_name, TextUtils::directedText(_role, '[', ']')));
    m_ui->userName->setToolTip(_email);
    //
    // Настриваем выравнивание, чтобы все строки были выровнены
    // по одному краю, вне зависимости от языка на котором они написаны
    //
    if ((isRightToLeft() && !_name.isRightToLeft())
        || (!isRightToLeft() && _name.isRightToLeft())) {
        m_ui->userName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
}

void ProjectUserWidget::setDeletable(bool _isDeletable)
{
    m_ui->closeAccess->setEnabled(_isDeletable);
    if (!_isDeletable) {
        m_ui->closeAccess->setIcons(QIcon(":/Graphics/Iconset/account.png"));
    }
}

void ProjectUserWidget::initView()
{
    m_ui->closeAccess->setIcons(m_ui->closeAccess->icon());
#ifndef MOBILE_OS
    m_ui->mainLayout->setContentsMargins(10, 0, 10, 0);
    setMinimumHeight(40);
#endif
}

void ProjectUserWidget::initConnections()
{
    connect(m_ui->closeAccess, &QToolButton::clicked, [=] {
        emit removeUserRequested(m_ui->userName->toolTip());
    });
}
