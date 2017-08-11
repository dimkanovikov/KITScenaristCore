#include "ScenarioNavigatorProxyStyle.h"

#include <3rd_party/Helpers/StyleSheetHelper.h>

#include <QApplication>
#include <QPainter>
#include <QStyleOption>

using UserInterface::ScenarioNavigatorProxyStyle;


ScenarioNavigatorProxyStyle::ScenarioNavigatorProxyStyle(QStyle* _style)
    : QProxyStyle(_style)
{
}

void ScenarioNavigatorProxyStyle::drawPrimitive(QStyle::PrimitiveElement _element,
    const QStyleOption* _option, QPainter* _painter, const QWidget* _widget) const
{
    if (_element == QStyle::PE_IndicatorItemViewItemDrop) {
        _painter->setRenderHint(QPainter::Antialiasing, true);

        QPalette palette(QApplication::palette());
        QColor indicatorColor(palette.text().color());

        //
        // Кисть для рисования линий
        //
        QPen pen(indicatorColor);
        pen.setWidth(2);
        QBrush brush(indicatorColor);

        //
        // Настраиваем рисовальщика
        //
        _painter->setPen(pen);
        _painter->setBrush(brush);

        //
        // Элемент вставляется в конец списка
        //
        if (_option->rect.topLeft().isNull() && _option->rect.size().isEmpty()) {
            //
            // Рисуем вспомогательный треугольник внизу виджета
            //
            int x = _widget->width() / 2;
            int y = _widget->height() - 16;
            QPolygonF treangle;
            treangle <<  QPointF(x, y)
                     << QPointF(x + 7,  y + 10)
                     << QPointF(x - 7,  y + 10);
            _painter->drawPolygon(treangle);
        }
        //
        // Элемент вставляется между двух соседних
        //
        else if (!_option->rect.topLeft().isNull() && _option->rect.size().isEmpty()) {
            //
            // Рисуем линию между элементов
            //
            _painter->drawLine(QPoint(_option->rect.topLeft().x() - 10,
                                     _option->rect.topLeft().y()),
                              _option->rect.topRight());
            //
            // Вспомогательный треугольник
            //
            QPolygonF treangle;
            treangle <<  QPointF(_option->rect.topLeft().x() - 10, _option->rect.topLeft().y() - 4)
                     << QPointF(_option->rect.topLeft().x() - 5,  _option->rect.topLeft().y())
                     << QPointF(_option->rect.topLeft().x() - 10, _option->rect.topLeft().y() + 4);
            _painter->drawPolygon(treangle);
        }
        //
        // Элемент вставляется в группирующий
        //
        else {
            //
            // Заливку делаем полупрозрачной
            //
            indicatorColor.setAlpha(50);
            QBrush brush(indicatorColor);
            _painter->setBrush(brush);

            //
            // Расширим немного область, чтобы не перекрывать иконку
            //
            QRect rect = _option->rect;
            rect.setX(rect.topLeft().x() - 4);
            _painter->drawRoundedRect(rect, 2, 2);
        }
    }
#ifdef MOBILE_OS
    //
    // Рисуем индикатор элемента, если у него есть дети
    //
    else if (_element == QStyle::PE_IndicatorBranch
               && _option->state & QStyle::State_Children) {
        _painter->setRenderHint(QPainter::Antialiasing, true);

        QPalette palette(QApplication::palette());
        QColor indicatorColor(palette.text().color());

        //
        // Кисть для рисования линий
        //
        QPen pen(indicatorColor);
        pen.setWidth(1);
        pen.setJoinStyle(Qt::MiterJoin);
        QBrush brush(indicatorColor);

        //
        // Настраиваем рисовальщика
        //
        _painter->setPen(pen);
        _painter->setBrush(brush);

        const qreal arrowHeight = StyleSheetHelper::dpToPx(8.);
        const qreal arrowHalfWidth = StyleSheetHelper::dpToPx(7.);
        //
        // ... открытый
        //
        if (_option->state & QStyle::State_Open) {
            int x = _option->rect.center().x();
            int y = _option->rect.center().y() + arrowHeight/2;
            QPolygonF treangle;
            treangle <<  QPointF(x, y)
                     << QPointF(x - arrowHalfWidth,  y - arrowHeight)
                     << QPointF(x + arrowHalfWidth,  y - arrowHeight);
            _painter->drawPolygon(treangle);
        }
        //
        // ... закрытый
        //
        else {
            int x = _option->rect.center().x() + arrowHeight/2;
            int y = _option->rect.center().y();
            QPolygonF treangle;
            treangle <<  QPointF(x, y)
                     << QPointF(x - arrowHeight,  y - arrowHalfWidth)
                     << QPointF(x - arrowHeight,  y + arrowHalfWidth);
            _painter->drawPolygon(treangle);
        }
    }
#endif
    else {
        QProxyStyle::drawPrimitive(_element, _option, _painter, _widget);
    }
}
