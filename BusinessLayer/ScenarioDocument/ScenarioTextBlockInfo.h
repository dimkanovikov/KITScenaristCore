#ifndef SCENARIOTEXTBLOCKINFO_H
#define SCENARIOTEXTBLOCKINFO_H

#include <QTextBlockUserData>

namespace BusinessLogic
{
    /**
     * @brief Базовый класс с информацией о текстовом блоке
     */
    class TextBlockInfo : public QTextBlockUserData
    {
    public:
        /**
         * @brief Тип блока при сравнении сценариев
         */
        enum DiffType {
            kDiffEqual,
            kDiffAdded,
            kDiffRemoved
        };

    public:
        TextBlockInfo() = default;

        /**
         * @brief Установлена ли на блоке закладка
         */
        bool hasBookmark() const;

        /**
         * @brief Установить/снять закладку с блока
         */
        void setHasBookmark(bool _hasBookmark);

        /**
         * @brief Получить текст закладки
         */
        QString bookmark() const;

        /**
         * @brief Установить текст закладки
         */
        void setBookmark(const QString& _bookmark);

        /**
         * @brief Получить цвет закладки
         */
        QColor bookmarkColor() const;

        /**
         * @brief Установить цвет закладки
         */
        void setBookmarkColor(const QColor& _color);

        /**
         * @brief Получить цвет фона, при сравнении документов
         */
        QColor diffColor() const;

        /**
         * @brief Задать цвет фона, при сравнении документов
         */
        void setDiffType(DiffType _type);

    private:
        /**
         * @brief Установлена ли закладка для блока
         */
        bool m_hasBookmark = false;

        /**
         * @brief Текст закладки для блока
         */
        QString m_bookmark;

        /**
         * @brief Цвет закладки
         */
        QColor m_bookmarkColor;

        /**
         * @brief Цвет фона, при сравнении документов
         */
        QColor m_diffColor;
    };

    /**
     * @brief Класс для хранения информации о сцене
     */
    class SceneHeadingBlockInfo : public TextBlockInfo
    {
    public:
        SceneHeadingBlockInfo(const QString& _uuid = QString());

        /**
         * @brief Идентификатор сцены
         */
        QString uuid() const;

        /**
         * @brief Установить идентификатор
         */
        void setUuid(const QString& _uuid);

        /**
         * @brief Сформировать новый идентификатор
         */
        void rebuildUuid();

        /**
         * @brief Получить номер сцены
         */
        QString sceneNumber() const;

        /**
         * @brief Установить номер сцены
         */
        void setSceneNumber(const QString& _number);

        /**
         * @brief Зафиксирован ли номер сцены
         */
        bool isSceneNumberFixed() const;

        /**
         * @brief Задать фиксацию номера сцены
         */
        void setSceneNumberFixed(bool _fixed);

        /**
         * @brief Группа фиксации
         */
        unsigned sceneNumberFixNesting() const;

        /**
         * @brief Задать группу фиксации
         */
        void setSceneNumberFixNesting(unsigned sceneNumberFixNesting);

        /**
         * @brief Номер в группе фиксации
         */
        int sceneNumberSuffix() const;

        /**
         * @brief Задать номер в группе фиксации
         */
        void setSceneNumberSuffix(int sceneNumberSuffix);

        /**
         * @brief Получить цвета сцены
         */
        QString colors() const;

        /**
         * @brief Установить цвета сцены
         */
        void setColors(const QString& _colors);

        /**
         * @brief Получить штамп сцены
         */
        QString stamp() const;

        /**
         * @brief Установить штамп сцены
         */
        void setStamp(const QString& _stamp);

        /**
         * @brief Получить название сцены
         */
        QString title() const;

        /**
         * @brief Установить название сцены
         */
        void setTitle(const QString& _title);

        /**
         * @brief Получить описание
         */
        QString description() const;

        /**
         * @brief Установить описание
         */
        void setDescription(const QString& _description);

        /**
         * @brief Создать дубликат
         */
        SceneHeadingBlockInfo* clone() const;

    private:
        /**
         * @brief Идентификатор сцены
         */
        QString m_uuid;

        /**
         * @brief Номер сцены
         */
        QString m_sceneNumber;

        /**
         * @brief Зафиксирован ли номер сцены
         */
        bool m_fixed = false;

        /**
         * @brief Вложенность фиксации номера сцены
         */
        /** {@ */
        unsigned m_sceneNumberFixNesting = 0;
        unsigned m_sceneNumberSuffix = 0;
        /** @} */

        /**
         * @brief Цвета сцены
         */
        QString m_colors;

        /**
         * @brief Штамп сцены
         */
        QString m_stamp;

        /**
         * @brief Название
         */
        QString m_title;

        /**
         * @brief Текст описания
         */
        QString m_description;
    };

    /**
     * @brief Класс хранящий информацию о блоке реплики
     */
    class CharacterBlockInfo : public TextBlockInfo
    {
    public:
        CharacterBlockInfo() = default;

        /**
         * @brief Получить номер реплики
         */
        int dialogueNumbder() const;

        /**
         * @brief Установить номер реплики
         */
        void setDialogueNumber(int _number);

        /**
         * @brief Создать дубликат
         */
        CharacterBlockInfo* clone() const;

    private:
        /**
         * @brief Порядковый номер реплики персонажа
         */
        int m_dialogueNumber = 0;
    };
}

#endif // SCENARIOTEXTBLOCKINFO_H
