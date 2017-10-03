#ifndef TREEVIEWPROXYSTYLE_H
#define TREEVIEWPROXYSTYLE_H

#include <QProxyStyle>


/**
 * @brief Стиль отрисовки дерева
 */
class TreeViewProxyStyle : public QProxyStyle
{
public:
    explicit TreeViewProxyStyle(QStyle* _style = 0);

    void drawPrimitive(PrimitiveElement _element, const QStyleOption* _option,
                       QPainter* _painter, const QWidget* _widget) const;
};

#endif // TREEVIEWPROXYSTYLE_H
