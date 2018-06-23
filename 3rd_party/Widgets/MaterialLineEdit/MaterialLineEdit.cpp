#include "MaterialLineEdit.h"

#include <QEvent>
#include <QInputMethodEvent>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>


/**
 * @brief Редактор текста, которому можно отключить корректировку позиции на экране
 * @note Используется на iOS, когда нужно разместить виджет независимо от других условий
 */
class LineEditWithFixedPosition : public QLineEdit
{
public:
    explicit LineEditWithFixedPosition(QWidget* _parent = nullptr) : QLineEdit(_parent) {}

    /**
     * @brief Установить необходимость корректировки позиции
     */
    void setNeedCorrectPosition(bool _needCorrect) {
        if (m_needCorrectPosition != _needCorrect) {
            m_needCorrectPosition = _needCorrect;
        }
    }

    /**
     * @brief Переопределяем для корректной работы позиционирования виджета на экране
     */
    /** @{ */
    QVariant inputMethodQuery(Qt::InputMethodQuery _property) const Q_DECL_OVERRIDE {
        return inputMethodQuery(_property, QVariant());
    }
    Q_INVOKABLE QVariant inputMethodQuery(Qt::InputMethodQuery _query, QVariant _argument) const {
        QVariant result = QLineEdit::inputMethodQuery(_query, _argument);
#ifdef Q_OS_IOS
        if (!m_needCorrectPosition) {
            //
            // Делаем курсор всегда на нуле, чтобы редактор сценария не выкидывало ни вниз ни вверх
            //
            if (_query & Qt::ImCursorRectangle) {
                result = QRectF(0,0,0,0);
            }
        }
#endif
        return result;
    }
    /** @} */

private:
    /**
     * @brief Необходимо ли корректировать позицию на экране
     */
    bool m_needCorrectPosition = true;
};


MaterialLineEdit::MaterialLineEdit(QWidget* _parent) :
    QWidget(_parent),
    m_label(new QLabel(this)),
    m_lineEdit(new LineEditWithFixedPosition(this)),
    m_helper(new QLabel(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(m_lineEdit);
    m_label->setFocusPolicy(Qt::StrongFocus);
    m_label->setFocusProxy(m_lineEdit);
    m_helper->setFocusPolicy(Qt::StrongFocus);
    m_helper->setFocusProxy(m_lineEdit);
    m_helper->setWordWrap(true);

    m_lineEdit->installEventFilter(this);
    m_lineEdit->setInputMethodHints(m_lineEdit->inputMethodHints()
                                    | Qt::ImhNoPredictiveText);

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
    return m_lineEdit->text();
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

void MaterialLineEdit::setInlineMode(bool _isInline)
{
    if (m_isInline != _isInline) {
        m_isInline = _isInline;

        updateStyle();
    }
}

void MaterialLineEdit::setNeedCorrectScreenPosition(bool _needCorrect)
{
    m_lineEdit->setNeedCorrectPosition(_needCorrect);
}

void MaterialLineEdit::setValidator(const QValidator* _validator)
{
    m_lineEdit->setValidator(_validator);
}

void MaterialLineEdit::setReadOnly(bool _readOnly)
{
    m_lineEdit->setReadOnly(_readOnly);
}

bool MaterialLineEdit::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == m_lineEdit) {
        if (_event->type() == QEvent::FocusIn
            || _event->type() == QEvent::FocusOut) {
            updatePlaceholder();
        }
    }

    return QWidget::eventFilter(_watched, _event);
}

void MaterialLineEdit::updateStyle()
{
    m_label->setVisible(!m_isInline);
    m_label->setProperty("materialLineEditLabel", !m_isError);
    m_helper->setVisible(!m_isInline);
    m_helper->setProperty("materialLineEditHelper", !m_isError);
    if (m_isInline) {
        m_lineEdit->setProperty("materialLineEdit", QVariant());
        m_lineEdit->setProperty("inlineInput", !m_isError);
    } else {
        m_lineEdit->setProperty("inlineInput", QVariant());
        m_lineEdit->setProperty("materialLineEdit", !m_isError);
    }

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
