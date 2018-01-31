#include "ClickableLabel.h"


ClickableLabel::ClickableLabel(QWidget *_parent) :
    QLabel(_parent)
{
    setCursor(Qt::PointingHandCursor);
}

void ClickableLabel::enterEvent(QEvent* _event)
{
    QFont font = this->font();
    font.setUnderline(true);
    setFont(font);

    QLabel::enterEvent(_event);
}

void ClickableLabel::leaveEvent(QEvent* _event)
{
    QFont font = this->font();
    font.setUnderline(false);
    setFont(font);

    QLabel::leaveEvent(_event);
}

void ClickableLabel::mousePressEvent(QMouseEvent* _event)
{
    emit clicked();

    QLabel::mousePressEvent(_event);
}
