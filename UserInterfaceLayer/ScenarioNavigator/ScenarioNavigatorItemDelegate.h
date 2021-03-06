#ifndef SCENARIONAVIGATORITEMDELEGATE_H
#define SCENARIONAVIGATORITEMDELEGATE_H

#include <QStyledItemDelegate>


namespace UserInterface {
    /**
    * @brief Делегат для отрисовки элементов навигатора
    */
    class ScenarioNavigatorItemDelegate : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        explicit ScenarioNavigatorItemDelegate(QObject* _parent = 0);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const;
        QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

        /**
        * @brief Настроить способ отображения
        */
        /** @{ */
        void setShowSceneNumber(bool _show);
        void setShowSceneTitle(bool _show);
        void setShowSceneDescription(bool _show);
        void setSceneDescriptionIsSceneText(bool _isSceneText);
        void setSceneDescriptionHeight(int _height);
        void setSceneNumbersPrefix(const QString& _prefix);
        /** @} */

    private:
        /**
        * @brief Отображать номер сцены
        */
        bool m_showSceneNumber;

        /**
        * @brief Отображать название сцены
        */
        bool m_showSceneTitle;

        /**
        * @brief Отображать описание сцены
        */
        bool m_showSceneDescription;

        /**
        * @brief Описанием сцены является текст сцены (true) или синопсис (false)
        */
        bool m_sceneDescriptionIsSceneText;

        /**
        * @brief Высота поля для отображения описания сцены
        */
        int m_sceneDescriptionHeight;

        /**
         * @brief Префикс номеров сцен
         */
        QString m_sceneNumbersPrefix;

        /**
         * @brief Константы для вью.
         * Нельзя инициализировать хелпером статически
         */
        /** @{ */
        int m_iconSize;
        const int m_iconTopMargin;
        const int m_topMargin;
        const int m_bottomMargin;
        int m_itemsHorizontalSpacing;
        const int m_itemsVerticalSpacing;
        /** @} */
    };
}

#endif // SCENARIONAVIGATORITEMDELEGATE_H
