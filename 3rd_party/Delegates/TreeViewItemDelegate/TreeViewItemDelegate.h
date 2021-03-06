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

    /**
     * @brief Переопределяем для ручной отрисовки элементов
     */
    void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

    /**
     * @brief Переопределяем для задания необходимого размера элементам
     */
    QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

    /**
     * @brief Переопределяем для реализации установки/снятия галочек с элементов
     */
    bool editorEvent(QEvent* _event, QAbstractItemModel* _model, const QStyleOptionViewItem& _option, const QModelIndex& _index) override;

    /**
     * @brief Установить необходимость корректировки текста иконки
     */
    void setNeedColorize(bool _need);

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
    const int m_checkMarkSize;
    /** @} */

    /**
     * @brief Необходимо ли менять цвет иконки в соответствии с цветом текста
     */
    bool m_needColorize = true;
};

#endif // TREEVIEWITEMDELEGATE_H
