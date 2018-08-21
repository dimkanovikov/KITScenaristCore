#include "ScriptVersionWidget.h"
#include "ui_ScriptVersionWidget.h"

#include <3rd_party/Helpers/StyleSheetHelper.h>

using UserInterface::ScriptVersionWidget;


ScriptVersionWidget::ScriptVersionWidget(bool _isFirstVersion, QWidget *parent) :
    QFrame(parent),
    m_ui(new Ui::ScriptVersionWidget),
    m_isFirstVersion(_isFirstVersion)
{
    m_ui->setupUi(this);

    initView();
    initConnections();
    initStylesheet();
}

ScriptVersionWidget::~ScriptVersionWidget()
{
    delete m_ui;
}

void UserInterface::ScriptVersionWidget::setColor(const QColor& _color)
{
    const int size = 16;
    const QString styleSheet
            = QString("margin-top: %1dp; "
                      "min-width: %1dp; max-width: %1dp; "
                      "min-height: %1dp; max-height: %1dp; "
                      "border-radius: %2dp; "
                      "background-color: %3;")
            .arg(size)
            .arg(size / 2)
            .arg(_color.name());
    m_ui->color->setStyleSheet(StyleSheetHelper::computeDeviceInpedendentSize(styleSheet));
}

void ScriptVersionWidget::setTitle(const QString& _title)
{
    m_ui->title->setText(_title);
}

void ScriptVersionWidget::setDescription(const QString& _description)
{
    m_ui->Description->setText(_description);
}

#ifndef MOBILE_OS
void ScriptVersionWidget::enterEvent(QEvent* _event)
{
    setMouseHover(true);
    QFrame::enterEvent(_event);
}

void ScriptVersionWidget::leaveEvent(QEvent* _event)
{
    setMouseHover(false);
    QFrame::leaveEvent(_event);
}
#endif

void ScriptVersionWidget::initView()
{
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    setMouseHover(false);

    m_ui->remove->setIcons(m_ui->remove->icon());
}

void ScriptVersionWidget::initConnections()
{
    connect(m_ui->remove, &FlatButton::clicked, this, &ScriptVersionWidget::removeClicked);
}

void ScriptVersionWidget::initStylesheet()
{
    setProperty("projectFrame", true);
    m_ui->frame->setProperty("projectTextFrame", true);
    m_ui->remove->setProperty("projectAction", true);
}

void ScriptVersionWidget::setMouseHover(bool _hover)
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

    //
    // Показываем, или скрываем кнопки параметров,
    // для первой версии кнопки не отображаются
    //
    m_ui->remove->setVisible(!m_isFirstVersion && _hover);
}
