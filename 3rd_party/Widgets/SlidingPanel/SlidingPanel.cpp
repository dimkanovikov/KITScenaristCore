#include "SlidingPanel.h"

#include <QResizeEvent>


SlidingPanel::SlidingPanel(QWidget* _parent) :
    QFrame(_parent)
{
}

void SlidingPanel::setFixedCornerPos(const QPoint& _pos, Qt::Corner _corner)
{
    m_pos = _pos;
    m_corner = _corner;

    correctPosition(size());
}

void SlidingPanel::resizeEvent(QResizeEvent* _event)
{
    correctPosition(_event->size());

    QFrame::resizeEvent(_event);
}

void SlidingPanel::correctPosition(const QSize& _size)
{
    switch (m_corner) {
        case Qt::TopRightCorner: {
            move(m_pos.x() - _size.width(), m_pos.y());
            break;
        }

        case Qt::BottomLeftCorner: {
            move(m_pos.x(), m_pos.y() - _size.height());
            break;
        }

        default: break;
    }
}
