#ifndef SCENARIOTEXTBLOCKINFO_H
#define SCENARIOTEXTBLOCKINFO_H

#include <QTextBlockUserData>

namespace BusinessLogic
{
    /**
     * @brief Класс для хранения информации о сцене
     */
    class SceneHeadingBlockInfo : public QTextBlockUserData
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
        int sceneNumber() const;

        /**
         * @brief Установить номер сцены
         */
        void setSceneNumber(int _number);

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
        int m_sceneNumber;

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
    class CharacterBlockInfo : public QTextBlockUserData
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
