#ifndef SCRIPTVERSION_H
#define SCRIPTVERSION_H

#include "DomainObject.h"

#include <QColor>
#include <QDateTime>
#include <QString>


namespace Domain
{
    /**
     * @brief Класс версии сценария
     */
    class ScriptVersion : public DomainObject
    {
    public:
        ScriptVersion(const Identifier& _id, const QString& _username, const QDateTime& _datetime, const QColor& _color,
                      const QString& _name, const QString& _description, const QString& _scriptText);

        /**
         * @brief Получить имя пользовтеля сохранившего версию
         */
        QString username() const;

        /**
         * @brief Задать имя пользовтеля сохранившего версию
         */
        void setUsername(const QString& _username);

        /**
         * @brief Получить дату версии
         */
        QDateTime datetime() const;

        /**
         * @brief Установить дату версии
         */
        void setDatetime(const QDateTime& _datetime);

        /**
         * @brief Получить цвет версии
         */
        QColor color() const;

        /**
         * @brief Установить цвет версии
         */
        void setColor(const QColor& _color);

        /**
         * @brief Получить название версии
         */
        QString name() const;

        /**
         * @brief Установить название версии
         */
        void setName(const QString& _name);

        /**
         * @brief Получить описание версии
         */
        QString description() const;

        /**
         * @brief Установить описание версии
         */
        void setDescription(const QString& _description);

        /**
         * @brief Получить текст сценария версии
         */
        QString scriptText() const;

        /**
         * @brief Задать текст сценария версии
         */
        void setScriptText(const QString& _scriptText);

    private:
        /**
         * @brief Имя пользователя
         */
        QString m_username;

        /**
         * @brief Дата версии
         */
        QDateTime m_datetime;

        /**
         * @brief Цвет версии
         */
        QColor m_color;

        /**
         * @brief Название версии
         */
        QString m_name;

        /**
         * @brief Описание версии
         */
        QString m_description;

        /**
         * @brief Текст сценария
         */
        QString m_scriptText;
    };

    // ****

    class ScriptVersionsTable : public DomainObjectsItemModel
    {
        Q_OBJECT

    public:
        explicit ScriptVersionsTable(QObject* _parent = nullptr);

    public:
        enum Column {
            kUndefined,
            kId,
            kUsername,
            kDatetime,
            kColor,
            kName,
            kDescription,
            kScriptText,
            kColumnCount
        };

    public:
        int columnCount(const QModelIndex&) const;
        QVariant data(const QModelIndex& _index, int _role) const;
    };
}

#endif // SCRIPTVERSION_H
