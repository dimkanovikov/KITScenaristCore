#include "ClickableFrame.h"


ClickableFrame::ClickableFrame(QWidget *parent) :
    QFrame(parent)
{
    setCursor(Qt::PointingHandCursor);
}

void ClickableFrame::mouseReleaseEvent(QMouseEvent* _event)
{
    QFrame::mouseReleaseEvent(_event);

    emit clicked();
}
