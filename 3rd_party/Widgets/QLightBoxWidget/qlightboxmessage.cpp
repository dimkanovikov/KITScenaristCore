#include "qlightboxmessage.h"

#include <3rd_party/Helpers/StyleSheetHelper.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVariant>
#include <QVBoxLayout>

namespace {
    /**
     * @brief Размер иконки
     */
    const QSize ICON_PIXMAP_SIZE(48, 48);
}


QDialogButtonBox::StandardButton QLightBoxMessage::critical(QWidget* _parent, const QString& _title,
    const QString& _text, QDialogButtonBox::StandardButtons _buttons,
    QDialogButtonBox::StandardButton _defaultButton, const QVector<ButtonInfo>& _buttonInfos)
{
    auto message = prepareMessage(_parent, _title, _text, QStyle::SP_MessageBoxCritical, _buttons, _defaultButton, _buttonInfos);
    return execMessage(message);
}

QDialogButtonBox::StandardButton QLightBoxMessage::information(QWidget* _parent, const QString& _title,
    const QString& _text, QDialogButtonBox::StandardButtons _buttons,
    QDialogButtonBox::StandardButton _defaultButton, const QVector<ButtonInfo>& _buttonInfos)
{
    auto message = prepareMessage(_parent, _title, _text, QStyle::SP_MessageBoxInformation, _buttons, _defaultButton, _buttonInfos);
    return execMessage(message);
}

QDialogButtonBox::StandardButton QLightBoxMessage::question(QWidget* _parent, const QString& _title,
    const QString& _text, QDialogButtonBox::StandardButtons _buttons,
    QDialogButtonBox::StandardButton _defaultButton, const QVector<ButtonInfo>& _buttonInfos)
{
    auto message = prepareMessage(_parent, _title, _text, QStyle::SP_MessageBoxQuestion, _buttons, _defaultButton, _buttonInfos);
    return execMessage(message);
}

QDialogButtonBox::StandardButton QLightBoxMessage::warning(QWidget* _parent, const QString& _title,
    const QString& _text, QDialogButtonBox::StandardButtons _buttons,
    QDialogButtonBox::StandardButton _defaultButton, const QVector<ButtonInfo>& _buttonInfos)
{
    auto message = prepareMessage(_parent, _title, _text, QStyle::SP_MessageBoxWarning, _buttons, _defaultButton, _buttonInfos);
    return execMessage(message);
}

QLightBoxMessage* QLightBoxMessage::showCritical(QWidget* _parent, const QString& _title, const QString& _text,
    QDialogButtonBox::StandardButtons _buttons, QDialogButtonBox::StandardButton _defaultButton,
    const QVector<QLightBoxMessage::ButtonInfo>& _buttonInfos)
{
    auto message = prepareMessage(_parent, _title, _text, QStyle::SP_MessageBoxCritical, _buttons, _defaultButton, _buttonInfos);
    return showMessage(message);
}

QLightBoxMessage* QLightBoxMessage::showInformation(QWidget* _parent, const QString& _title, const QString& _text,
    QDialogButtonBox::StandardButtons _buttons, QDialogButtonBox::StandardButton _defaultButton,
    const QVector<QLightBoxMessage::ButtonInfo>& _buttonInfos)
{
    auto message = prepareMessage(_parent, _title, _text, QStyle::SP_MessageBoxInformation, _buttons, _defaultButton, _buttonInfos);
    return showMessage(message);
}

QLightBoxMessage* QLightBoxMessage::showQuestion(QWidget* _parent, const QString& _title,
    const QString& _text, QDialogButtonBox::StandardButtons _buttons, QDialogButtonBox::StandardButton _defaultButton,
    const QVector<QLightBoxMessage::ButtonInfo>& _buttonInfos)
{
    auto message = prepareMessage(_parent, _title, _text, QStyle::SP_MessageBoxQuestion, _buttons, _defaultButton, _buttonInfos);
    return showMessage(message);
}

QLightBoxMessage* QLightBoxMessage::showWarning(QWidget* _parent, const QString& _title, const QString& _text,
    QDialogButtonBox::StandardButtons _buttons, QDialogButtonBox::StandardButton _defaultButton,
    const QVector<QLightBoxMessage::ButtonInfo>& _buttonInfos)
{
    auto message = prepareMessage(_parent, _title, _text, QStyle::SP_MessageBoxWarning, _buttons, _defaultButton, _buttonInfos);
    return showMessage(message);
}

QDialogButtonBox::StandardButton QLightBoxMessage::execMessage(QLightBoxMessage* _message)
{
    QSharedPointer<QLightBoxMessage> message{_message};
    const int execResult = message->exec();
    return static_cast<QDialogButtonBox::StandardButton>(execResult);
}

QLightBoxMessage*QLightBoxMessage::showMessage(QLightBoxMessage* _message)
{
    connect(_message, &QLightBoxMessage::finished, _message, &QLightBoxMessage::deleteLater);
    _message->show();
    return _message;
}

QLightBoxMessage* QLightBoxMessage::prepareMessage(QWidget* _parent, const QString& _title, const QString& _text,
    QStyle::StandardPixmap _pixmap, QDialogButtonBox::StandardButtons _buttons,
    QDialogButtonBox::StandardButton _defaultButton, const QVector<QLightBoxMessage::ButtonInfo>& _buttonInfos)
{
    QLightBoxMessage* message = new QLightBoxMessage(_parent);
    message->setWindowTitle(_title);
    message->m_icon->setPixmap(message->style()->standardIcon(_pixmap).pixmap(ICON_PIXMAP_SIZE));
    message->m_text->setText(_text);
    message->m_buttons->setStandardButtons(_buttons);
    if (_buttons.testFlag(_defaultButton)) {
        message->m_buttons->button(_defaultButton)->setDefault(true);
    }

    for (QAbstractButton* button : message->m_buttons->buttons())	{
        button->setProperty("flat", true);
#ifdef MOBILE_OS
        //
        // Для мобильных делаем кнопки в верхнем регистре и убераем ускорители
        //
        button->setText(button->text().toUpper().remove("&"));
#endif
    }

    //
    // Задаем названия кнопок, если есть
    //
    for (const ButtonInfo& buttonInfo : _buttonInfos) {
        message->m_buttons->button(buttonInfo.type)->setText(buttonInfo.name);
    }

    return message;
}

QLightBoxMessage::QLightBoxMessage(QWidget* _parent) :
    QLightBoxDialog(_parent),
    m_icon(new QLabel(this)),
    m_text(new QLabel(this)),
    m_buttons(new QDialogButtonBox(this))
{
}

void QLightBoxMessage::initView()
{
    m_icon->setProperty("lightBoxMessageIcon", true);
    m_icon->setFixedSize(ICON_PIXMAP_SIZE);
#ifdef MOBILE_OS
    m_icon->hide();
#endif

    m_text->setProperty("lightBoxMessageText", true);
    m_text->setWordWrap(true);

    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(QMargins());
    topLayout->setSpacing(20);
    topLayout->addWidget(m_icon, 0, Qt::AlignLeft | Qt::AlignTop);
    topLayout->addWidget(m_text, 1);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    layout->addLayout(topLayout);
    layout->addWidget(m_buttons);
    setLayout(layout);

#ifndef MOBILE_OS
    setMinimumWidth(500);
#else
    setMinimumWidth(qMin(StyleSheetHelper::dpToPx(400),
                         parentWidget()->width() - StyleSheetHelper::dpToPx(56)));
#endif
}

void QLightBoxMessage::initConnections()
{
    connect(m_buttons, SIGNAL(clicked(QAbstractButton*)), this, SLOT(aboutClicked(QAbstractButton*)));
}

QWidget* QLightBoxMessage::focusedOnExec() const
{
    QWidget* focusedOn = m_buttons;
    if (m_buttons->standardButtons().testFlag(QDialogButtonBox::Ok)) {
        focusedOn = m_buttons->button(QDialogButtonBox::Ok);
    } else if (m_buttons->standardButtons().testFlag(QDialogButtonBox::Open)) {
        focusedOn = m_buttons->button(QDialogButtonBox::Open);
    } else if (m_buttons->standardButtons().testFlag(QDialogButtonBox::Save)) {
        focusedOn = m_buttons->button(QDialogButtonBox::Save);
    } else if (m_buttons->standardButtons().testFlag(QDialogButtonBox::Yes)) {
        focusedOn = m_buttons->button(QDialogButtonBox::Yes);
    }
    return focusedOn;
}

void QLightBoxMessage::aboutClicked(QAbstractButton* _button)
{
    done(m_buttons->standardButton(_button));
}

