#ifndef TREEVIEWITEMDELEGATE_H
#define TREEVIEWITEMDELEGATE_H

#include <QStyledItemDelegate>

/**
 * @brief Делегат для отрисовки элементов дерева
 */
class TreeViewItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit TreeViewItemDelegate(QObject* _parent = 0);

    void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const;
    QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

private:
    /**
     * @brief Константы для вью.
     * Нельзя инициализировать хелпером статически
     */
    /** @{ */
    const int m_iconSize;
    const int m_topMargin;
    const int m_bottomMargin;
    const int m_itemsSpacing;
    /** @} */
};

#endif // TREEVIEWITEMDELEGATE_H
