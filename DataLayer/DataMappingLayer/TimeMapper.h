#ifndef TIMEMAPPER_H
#define TIMEMAPPER_H

#include "AbstractMapper.h"
#include "MapperFacade.h"

namespace Domain {
	class SceneTime;
	class SceneTimesTable;
}

using namespace Domain;


namespace DataMappingLayer
{
	class TimeMapper : public AbstractMapper
	{
	public:
		SceneTime* find(const Identifier& _id);
		SceneTimesTable* findAll();
		void insert(SceneTime* _time);
		void update(SceneTime* _time);
        void remove(SceneTime* _time);

	protected:
		QString findStatement(const Identifier& _id) const;
		QString findAllStatement() const;
		QString insertStatement(DomainObject* _subject, QVariantList& _insertValues) const;
		QString updateStatement(DomainObject* _subject, QVariantList& _updateValues) const;
		QString deleteStatement(DomainObject* _subject, QVariantList& _deleteValues) const;

	protected:
		DomainObject* doLoad(const Identifier& _id, const QSqlRecord& _record);
		void doLoad(DomainObject* _domainObject, const QSqlRecord& _record);
		DomainObjectsItemModel* modelInstance();

	private:
		TimeMapper();

		// Для доступа к конструктору
		friend class MapperFacade;
	};
}

#endif // TIMEMAPPER_H
