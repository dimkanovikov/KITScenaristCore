#ifndef TRANSITION_H
#define TRANSITION_H

#include "DomainObject.h"

#include <QString>


namespace Domain
{
    /**
     * @brief Класс перехода
     */
    class Transition : public DomainObject
    {
    public:
        Transition(const Identifier& _id, const QString& _name);

        /**
         * @brief Получить название перехода
         */
        QString name() const;

        /**
         * @brief Установить название перехода
         */
        void setName(const QString& _name);

        /**
         * @brief Проверить на равенство
         */
        bool equal(const QString& _name) const;

    private:
        /**
         * @brief Название перехода
         */
        QString m_name;
    };

    // ****

    class TransitionsTable : public DomainObjectsItemModel
    {
        Q_OBJECT

    public:
        explicit TransitionsTable(QObject* _parent = 0);

    public:
        enum Column {
            Undefined,
            Id,
            Name
        };

    public:
        int columnCount(const QModelIndex&) const;
        QVariant data(const QModelIndex& _index, int _role) const;

    private:
        Column sectionToColumn(int _section) const;
    };
}

#endif // TRANSITION_H
