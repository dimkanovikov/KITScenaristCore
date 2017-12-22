#ifndef CHARACTERSTATESTORAGE_H
#define CHARACTERSTATESTORAGE_H

#include "StorageFacade.h"

class QString;

namespace Domain {
	class CharacterState;
	class CharacterStatesTable;
}

using namespace Domain;


namespace DataStorageLayer
{
	class CharacterStateStorage
	{
	public:
		/**
         * @brief Получить все состояния
		 */
		CharacterStatesTable* all();

		/**
         * @brief Сохранить состояние
		 */
		CharacterState* storeCharacterState(const QString& _characterStateName);

		/**
         * @brief Проверить наличие заданного состояния
		 */
		bool hasCharacterState(const QString& _name);

        /**
         * @brief Удалить состояние
         */
        void removeCharacterState(const QString& _name);

		/**
		 * @brief Очистить хранилище
		 */
		void clear();

		/**
		 * @brief Обновить хранилище
		 */
		void refresh();

	private:
		CharacterStatesTable* m_all;

	private:
		CharacterStateStorage();

		// Для доступа к конструктору
		friend class StorageFacade;
	};
}

#endif // CHARACTERSTATESTORAGE_H
