#ifndef SCENARIODOCUMENT_H
#define SCENARIODOCUMENT_H

#include <QObject>
#include <QMap>
#include <QUuid>

class QTextDocument;
class QAbstractItemModel;

namespace Domain {
    class Scenario;
}

namespace BusinessLogic
{
    class ScenarioXml;
    class ScenarioTextDocument;
    class ScenarioModel;
    class ScenarioModelItem;


    /**
     * @brief Класс документа сценария
     *
     * Содержит в себе как текст документа, так и его древовидную модель, которые
     * синхронизируются при редактировании одного из них
     */
    class ScenarioDocument : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief Майм-тип данных сценариев
         */
        static QString MIME_TYPE;

    public:
        explicit ScenarioDocument(QObject* _parent = 0);

        /**
         * @brief Текстовый документ
         */
        ScenarioTextDocument* document() const;

        /**
         * @brief Модель сценария
         */
        ScenarioModel* model() const;

        /**
         * @brief Количество сцен в сценарии
         */
        int scenesCount() const;

        /**
         * @brief Посчитать длительность сценария до указанной позиции
         */
        qreal durationAtPosition(int _position) const;

        /**
         * @brief Длительность всего сценария
         */
        int fullDuration() const;

        /**
         * @brief Показания счётчиков
         */
        QStringList countersInfo() const;

        /**
         * @brief Индекс элемента дерева в указанной позиции
         */
        QModelIndex itemIndexAtPosition(int _position) const;

        /**
         * @brief Позиция начала сцены
         */
        int itemStartPosition(const QModelIndex& _index) const;

        /**
         * @brief Позиция внутри папки, если вызывается для сцены, то возвращается конец сцены
         */
        int itemMiddlePosition(const QModelIndex& _index) const;

        /**
         * @brief Позиция конца сцены
         */
        int itemEndPosition(const QModelIndex& _index) const;

        /**
         * @brief Заголовок сцены в позиции
         */
        QString itemHeaderAtPosition(int _position) const;

        /**
         * @brief Идентификатор сцены
         */
        QString itemUuid(ScenarioModelItem* _item) const;

        /**
         * @brief Цвета сцены
         */
        QString itemColors(ScenarioModelItem* _item) const;

        /**
         * @brief Номер сцены
         */
        QString itemSceneNumber(ScenarioModelItem* _item) const;

        /**
         * @brief Порядковый номер сцены в своей группе фиксации
         */
        int itemSceneNumberSuffix(ScenarioModelItem* _item) const;

        /**
         * @brief Зафиксирована ли сцена
         */
        bool itemSceneIsFixed(ScenarioModelItem* _item) const;

        /**
         * @brief Группа фиксации (сколько раз сцена была зафиксирована)
         */
        int itemSceneFixNesting(ScenarioModelItem* _item) const;

        /**
         * @brief Установить цвет для сцены в указанной позиции
         */
        void setItemColorsAtPosition(int _position, const QString& _colors);

        /**
         * @brief Штамп сцены
         */
        QString itemStamp(ScenarioModelItem* _item) const;

        /**
         * @brief Установить штамп для сцены в указанной позиции
         */
        void setItemStampAtPosition(int _position, const QString& _stamp);

        /**
         * @brief Название сцены в позиции
         */
        QString itemTitleAtPosition(int _position) const;

        /**
         * @brief Название сцены
         */
        QString itemTitle(ScenarioModelItem* _item) const;

        /**
         * @brief Установить название для сцены в указанной позиции курсора
         */
        void setItemTitleAtPosition(int _position, const QString& _title);

        /**
         * @brief Описание сцены в позиции
         */
        QString itemDescriptionAtPosition(int _position) const;

        /**
         * @brief Описание сцены
         */
        QString itemDescription(const ScenarioModelItem* _item) const;

        /**
         * @brief Установить описание для сцены в указанной позиции курсора
         */
        void setItemDescriptionAtPosition(int _position, const QString& _description);

        /**
         * @brief Копировать описание сцены в текст сценария
         */
        void copyItemDescriptionToScript(int _position);

        /**
         * @brief Добавить закладку
         */
        void addBookmark(int _position, const QString& _text, const QColor& _color);

        /**
         * @brief Удалить закладку
         */
        void removeBookmark(int _position);

        /**
         * @brief Загрузить документ из сценария
         */
        void load(Domain::Scenario* _scenario);

        /**
         * @brief Получить сценарий из которого загружен документ
         */
        Domain::Scenario* scenario() const;

        /**
         * @brief Установить сценарий
         */
        void setScenario(Domain::Scenario* _scenario);

        /**
         * @brief Сохранить документ в строку
         */
        QString save() const;

        /**
         * @brief Очистить сценарий
         */
        void clear();

        /**
         * @brief Обновить сценарий
         */
        void refresh();

        /**
         * @brief Найти всех персонажей сценария
         */
        QStringList findCharacters() const;

        /**
         * @brief Найти все локации сценария
         */
        QStringList findLocations() const;

        /**
         * @brief Обновить номера сцен в блоках информации документа
         */
        void updateDocumentScenesAndDialoguesNumbers();

        /**
         * @brief Зафиксировать номера сцен
         */
        void changeSceneNumbersLocking(bool _lock);

        /**
         * @brief Зафиксирована ли хоть одна сцена
         */
        bool isAnySceneLocked();

        /**
         * @brief Задать стартовый номер сцен
         */
        void setSceneStartNumber(int _startNumber);

        /**
         * @brief Задать новый номер сцены
         */
        void setNewSceneNumber(const QString& _newSceneNumber, int _position);

    public:
        /**
         * @brief Вспомогательные функции для обработчика xml
         *
         * FIXME: плохо, что эти функции являются открытыми их нужно перенести в закрытую часть класса
         */
        /** @{ */
        //! Определить позицию вставки майм-данных в документ
        int positionToInsertMime(ScenarioModelItem* _insertParent, ScenarioModelItem* _insertBefore) const;
        /** @} */

    signals:
        /**
         * @brief Изменился текст
         */
        void textChanged();

        /**
         * @brief Изменилась информация о зафиксированности сцен
         */
        void fixedScenesChanged(bool _anyFixed);

    private slots:
        /**
         * @brief Изменилось содержимое документа
         */
        void aboutContentsChange(int _position, int _charsRemoved, int _charsAdded);

        /**
         * @brief Скорректировать текст сценария
         */
        void correctText();

    private:
        /**
         * @brief Настроить необходимые соединения
         */
        void initConnections();

        /**
         * @brief Настроить соединения с TextDocument
         */
        void connectTextDocument();

        /**
         * @brief Отключить соединения с TextDocument
         */
        void disconnectTextDocument();

        /**
         * @brief Обновить элемент структуры из промежутка текста в который он входит
         */
        void updateItem(ScenarioModelItem* _item, int _itemStartPos, int _itemEndPos);

        /**
         * @brief Создать или получить существующий элемент для позиции в документе
         *		  или ближайший к позиции
         */
        ScenarioModelItem* itemForPosition(int _position, bool _findNear = false) const;

        /**
         * @brief Загрузить документ из сценария
         */
        void load(const QString& _scenario);

    private:
        /**
         * @brief Собственно сам сценарий
         */
        Domain::Scenario* m_scenario = nullptr;

        /**
         * @brief Обработчик xml
         */
        ScenarioXml* m_xmlHandler = nullptr;

        /**
         * @brief Документ сценария
         */
        ScenarioTextDocument* m_document = nullptr;

        /**
         * @brief Дерево сценария
         */
        ScenarioModel* m_model = nullptr;

        /**
         * @brief Карта элементов дерева сценария
         */
        QMap<int, ScenarioModelItem*> m_modelItems;

        /**
         * @brief Флаг операции обновления описания сцены, для предотвращения рекурсии
         */
        bool m_inSceneDescriptionUpdate = false;

        /**
         * @brief Параметры последнейго изменения текста сценария
         * @note Используется для корректировок текста после изменения текста документа
         */
        struct LastChangeOptions {
            /**
             * @brief Позиция изменения
             */
            int position = 0;

            /**
             * @brief Количество добавленных символов
             */
            int charactersAdded = 0;

            /**
             * @brief Количество удалённых символов
             */
            int charactersRemoved = 0;

            /**
             * @brief Есть ли необходимость в корректировках текста
             */
            bool needToCorrectText = false;
        } m_lastChange;
    };
}

#endif // SCENARIODOCUMENT_H
