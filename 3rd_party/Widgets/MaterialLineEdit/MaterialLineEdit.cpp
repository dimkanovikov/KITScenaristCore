#include "MaterialLineEdit.h"

#include <QEvent>
#include <QInputMethodEvent>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>


MaterialLineEdit::MaterialLineEdit(QWidget* _parent) :
    QWidget(_parent),
    m_label(new QLabel(this)),
    m_lineEdit(new QLineEdit(this)),
    m_helper(new QLabel(this))
{
    setFocusProxy(m_lineEdit);

    m_lineEdit->installEventFilter(this);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    layout->addWidget(m_label);
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_helper);
    setLayout(layout);

    connect(m_lineEdit, &QLineEdit::textChanged, this, &MaterialLineEdit::textChanged);

    updateStyle();
}

void MaterialLineEdit::clear()
{
    setText(QString());
    setError(false);
    setHelperText(QString());
}

void MaterialLineEdit::setLabel(const QString& _text)
{
    if (m_placeholder != _text) {
        m_placeholder = _text;
        updatePlaceholder();
    }
}

void MaterialLineEdit::setText(const QString& _text)
{
    m_text = _text;
    m_lineEdit->setText(m_text);
    updatePlaceholder();
}

QString MaterialLineEdit::text() const
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    if (m_lineEdit->echoMode() == QLineEdit::Password) {
        return m_lineEdit->text();
    } else  {
        return m_text;
    }
#else
    return m_lineEdit->text();
#endif
}

void MaterialLineEdit::setHelperText(const QString& _text)
{
    m_helper->setText(_text);
}

void MaterialLineEdit::setError(bool _isError)
{
    if (m_isError != _isError) {
        m_isError = _isError;
        updateStyle();
    }
}

void MaterialLineEdit::setEchoMode(QLineEdit::EchoMode _mode)
{
    m_lineEdit->setEchoMode(_mode);
}

void MaterialLineEdit::setUseEmailKeyboard(bool _use)
{
    if (_use) {
        m_lineEdit->setInputMethodHints(m_lineEdit->inputMethodHints() | Qt::ImhEmailCharactersOnly);
    }
}

bool MaterialLineEdit::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == m_lineEdit) {
        if (_event->type() == QEvent::FocusIn
            || _event->type() == QEvent::FocusOut) {
            updatePlaceholder();
        } else if (_event->type() == QEvent::InputMethod) {
            QInputMethodEvent* event = static_cast<QInputMethodEvent*>(_event);
            m_text = m_lineEdit->text();
            if (event->replacementStart() >= 0) {
                if (!event->preeditString().isEmpty()) {
                    m_text += event->preeditString();
                } else {
                    m_text += event->commitString();
                }
            }
            emit textChanged(m_text);
        }
    }

    return QWidget::eventFilter(_watched, _event);
}

void MaterialLineEdit::updateStyle()
{
    m_label->setProperty("materialLineEditLabel", !m_isError);
    m_lineEdit->setProperty("materialLineEdit", !m_isError);
    m_helper->setProperty("materialLineEditHelper", !m_isError);

    setStyleSheet("");
}

void MaterialLineEdit::updatePlaceholder()
{
    if (m_lineEdit->text().isEmpty()
        && !m_lineEdit->hasFocus()) {
        m_lineEdit->setPlaceholderText(m_placeholder);
        m_label->clear();
    } else {
        m_lineEdit->setPlaceholderText(QString());
        m_label->setText(m_placeholder);
    }
}
