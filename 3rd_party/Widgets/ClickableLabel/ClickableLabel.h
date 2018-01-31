#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>


/**
 * @brief Лейбл мимикрирующий под ссылку
 */
class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget* _parent = nullptr);

signals:
    /**
     * @brief Пользователь кликнул на метку
     */
    void clicked();

protected:
    /**
     * @brief Переопределяется для установки подчёркивания на текст
     */
    void enterEvent(QEvent* _event) override;

    /**
     * @brief Переопределяется для снятия подчёркивания с текста
     */
    void leaveEvent(QEvent* _event) override;

    /**
     * @brief Переопределяется для реализации собития щелчка
     */
    void mousePressEvent(QMouseEvent* _event) override;
};

#endif // CLICKABLELABEL_H
