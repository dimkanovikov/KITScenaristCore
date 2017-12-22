#ifndef TIMESTORAGE_H
#define TIMESTORAGE_H

#include "StorageFacade.h"

class QString;

namespace Domain {
	class SceneTime;
	class SceneTimesTable;
}

using namespace Domain;


namespace DataStorageLayer
{
	class TimeStorage
	{
	public:
		/**
		 * @brief Получить все времена
		 */
		SceneTimesTable* all();

		/**
		 * @brief Сохранить время
		 */
		SceneTime* storeTime(const QString& _timeName);

        /**
         * @brief Удалить время
         */
        void removeTime(const QString& _name);

		/**
		 * @brief Очистить хранилище
		 */
		void clear();

		/**
		 * @brief Обновить хранилище
		 */
		void refresh();

	private:
		SceneTimesTable* m_all;

	private:
		TimeStorage();

		// Для доступа к конструктору
		friend class StorageFacade;
	};
}

#endif // TIMESTORAGE_H
