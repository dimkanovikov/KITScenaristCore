#ifndef MATERIALLINEEDIT_H
#define MATERIALLINEEDIT_H

#include <QLineEdit>

class QLabel;


/**
 * @brief Поле для ввода текста в MaterialDesign стиле
 */
class MaterialLineEdit : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialLineEdit(QWidget* _parent = nullptr);

    /**
     * @brief Очистить поле ввода и убрать ошибку
     */
    void clear();

    /**
     * @brief Установить плейсхолдер в поле ввода
     */
    void setLabel(const QString& _text);

    /**
     * @brief Установить текст в поле ввода
     */
    void setText(const QString& _text);

    /**
     * @brief Получить текст из поля ввода
     */
    QString text() const;

    /**
     * @brief Установить текст подсказки
     */
    void setHelperText(const QString& _text);

    /**
     * @brief Установить ошибочное состояние
     */
    void setError(bool _isError);

    /**
     * @brief Установить режим отображения символов
     */
    void setEchoMode(QLineEdit::EchoMode _mode);

signals:
    /**
     * @brief Изменился текст в поле ввода
     */
    void textChanged(const QString& _text);

protected:
    /**
     * @brief Переопределяем, чтобы перехватывать событие фокуса поля ввода
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    /**
     * @brief Перенастроить внешний вид в соответствии с состоянием
     */
    void updateStyle();

    /**
     * @brief Обновить отображение плейсхолдера
     */
    void updatePlaceholder();

private:
    /**
     * @brief Заголовок
     */
    QLabel* m_label = nullptr;

    /**
     * @brief Поле для ввода текста
     */
    QLineEdit* m_lineEdit = nullptr;

    /**
     * @brief Подсказка
     */
    QLabel* m_helper = nullptr;

    /**
     * @brief Плейсхолдер для поля ввода
     */
    QString m_placeholder;

    /**
     * @brief Ошибочное состояние
     */
    bool m_isError = false;
};

#endif // MATERIALLINEEDIT_H
