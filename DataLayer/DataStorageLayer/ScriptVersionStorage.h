#ifndef SCRIPTVERSIONSTORAGE_H
#define SCRIPTVERSIONSTORAGE_H

#include "StorageFacade.h"

class QColor;
class QDateTime;
class QString;

namespace Domain {
    class ScriptVersion;
    class ScriptVersionsTable;
}

using namespace Domain;


namespace DataStorageLayer
{
    class ScriptVersionStorage
    {
    public:
        /**
         * @brief Получить все версии сценария
         */
        ScriptVersionsTable* all();

        /**
         * @brief Получить название текущей версии
         */
        QString currentVersionName();

        /**
         * @brief Сохранить версию
         */
        ScriptVersion* storeScriptVersion(const QDateTime& _datetime, const QColor& _color,
                                          const QString& _name, const QString& _description);

        /**
         * @brief Обновить версию сценария
         */
        void updateScriptVersion(ScriptVersion* _scriptVersion);

        /**
         * @brief Удалить версию сценария
         */
        void removeScriptVersion(ScriptVersion* _scriptVersion);

        /**
         * @brief Очистить хранилище
         */
        void clear();

        /**
         * @brief Обновить хранилище
         */
        void refresh();

    private:
        /**
         * @brief Список всех версий сценария
         */
        ScriptVersionsTable* m_all = nullptr;

    private:
        ScriptVersionStorage() = default;

        // Для доступа к конструктору
        friend class StorageFacade;
    };
}

#endif // SCRIPTVERSIONSTORAGE_H
