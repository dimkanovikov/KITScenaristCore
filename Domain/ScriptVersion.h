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
        ScriptVersion(const Identifier& _id, const QDateTime& _datetime, const QColor& _color,
                      const QString& _name, const QString& _description);

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

    private:
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
    };

    // ****

    class ScriptVersionsTable : public DomainObjectsItemModel
    {
        Q_OBJECT

    public:
        explicit ScriptVersionsTable(QObject* _parent = nullptr);

    public:
        enum Column {
            Undefined,
            Id,
            Datetime,
            Color,
            Name,
            Description
        };

    public:
        int columnCount(const QModelIndex&) const;
        QVariant data(const QModelIndex& _index, int _role) const;

    private:
        Column sectionToColumn(int _section) const;
    };
}

#endif // SCRIPTVERSION_H
