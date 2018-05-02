#ifndef SCRIPTBOOKMARKSMODEL_H
#define SCRIPTBOOKMARKSMODEL_H

#include <QAbstractListModel>
#include <QColor>


namespace BusinessLogic {
    class ScenarioTextDocument;

    /**
     * @brief Модель закладок документа
     */
    class ScriptBookmarksModel : public QAbstractListModel
    {
        Q_OBJECT

    public:
        /**
         * @brief Дополнительные роли для данных
         */
        enum BookmarkRoles {
            /**
              * @brief Цвет закладки
              */
            ColorRole = Qt::UserRole + 1
        };

    public:
        explicit ScriptBookmarksModel(ScenarioTextDocument* _parent);

        /**
         * @brief Реализация стандартных методов
         */
        /** @{ */
        int rowCount(const QModelIndex& _parent = QModelIndex()) const;
        QVariant data(const QModelIndex& _index, int _role) const;
        bool removeRows(int _row, int _count, const QModelIndex& _parent = QModelIndex());
        /** @} */

        /**
         * @brief Добавить закладку
         */
        void addBookmark(int _position, const QString& _text, const QColor& _color);

        /**
         * @brief Удалить закладку
         */
        void removeBookmark(int _position);

        /**
         * @brief Получить позицию закладки по заданному индексу
         */
        int positionForIndex(const QModelIndex& _index);

        /**
         * @brief Индекс элемента по позиции
         */
        QModelIndex indexForPosition(int _position);

    signals:
        /**
         * @brief Модель изменилась
         */
        void modelChanged();

    private:
        /**
         * @brief Обновить модель закладок
         */
        void aboutUpdateBookmarksModel(int _position, int _removed, int _added);

    private:
        /**
         * @brief Документ, по которому строится модель
         */
        ScenarioTextDocument* m_document = nullptr;

        /**
         * @brief Класс информации о закладке
         */
        class BookmarkInfo {
        public:
            BookmarkInfo() = default;

            /**
             * @brief Позиция
             */
            int position = 0;

            /**
             * @brief Текст
             */
            QString text;

            /**
             * @brief Цвет
             */
            QColor color;
        };

        /**
         * @brief Список закладок
         */
        QVector<BookmarkInfo> m_bookmarks;

        /**
         * @brief Карта закладок <позиция в документе, индекс закладки>
         */
        QMap<int, int> m_bookmarksMap;
    };
}

#endif // SCRIPTBOOKMARKSMODEL_H
