#ifndef TRANSITIONSTORAGE_H
#define TRANSITIONSTORAGE_H

#include "StorageFacade.h"

class QString;

namespace Domain {
    class Transition;
    class TransitionsTable;
}

using namespace Domain;


namespace DataStorageLayer
{
    class TransitionStorage
    {
    public:
        /**
         * @brief Получить все переходы
         */
        TransitionsTable* all();

        /**
         * @brief Сохранить время
         */
        Transition* storeTransition(const QString& _transitionName);

        /**
         * @brief Очистить хранилище
         */
        void clear();

        /**
         * @brief Обновить хранилище
         */
        void refresh();

    private:
        TransitionsTable* m_all = nullptr;

    private:
        TransitionStorage();

        // Для доступа к конструктору
        friend class StorageFacade;
    };
}

#endif // TRANSITIONSTORAGE_H
