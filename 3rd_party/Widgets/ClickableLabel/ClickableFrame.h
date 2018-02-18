#ifndef CLICKABLEFRAME_H
#define CLICKABLEFRAME_H

#include <QFrame>

class ClickableFrame : public QFrame
{
    Q_OBJECT

public:
    explicit ClickableFrame(QWidget *parent = nullptr);

signals:
    /**
     * @brief Пользователь кликнул на фрейме
     */
    void clicked();

protected:
    /**
     * @brief Переопределяем для испускания сигнала о клике
     */
    void mouseReleaseEvent(QMouseEvent* _event);
};

#endif // CLICKABLEFRAME_H
