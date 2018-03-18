#include "SlidingPanel.h"

#include <QResizeEvent>


SlidingPanel::SlidingPanel(QWidget* _parent) :
    QFrame(_parent)
{
}

void SlidingPanel::setFixedRightTopPos(const QPoint& _pos)
{
    m_pos = _pos;
    move(m_pos.x() - width(), m_pos.y());
}

void SlidingPanel::resizeEvent(QResizeEvent* _event)
{
    move(m_pos.x() - _event->size().width(), m_pos.y());
    QFrame::resizeEvent(_event);
}
