#include "TimeStorage.h"

#include <DataLayer/DataMappingLayer/MapperFacade.h>
#include <DataLayer/DataMappingLayer/TimeMapper.h>

#include <Domain/SceneTime.h>

#include <3rd_party/Helpers/TextEditHelper.h>

using namespace DataStorageLayer;
using namespace DataMappingLayer;


SceneTimesTable* TimeStorage::all()
{
	if (m_all == 0) {
		m_all = MapperFacade::timeMapper()->findAll();
	}
	return m_all;
}

SceneTime* TimeStorage::storeTime(const QString& _timeName)
{
    SceneTime* newTime = 0;

    const QString timeName = TextEditHelper::smartToUpper(_timeName).simplified();

	//
	// Если время можно сохранить
	//
	if (!timeName.isEmpty()) {
		//
		// Проверяем наличие данного времени
		//
		foreach (DomainObject* domainObject, all()->toList()) {
            SceneTime* time = dynamic_cast<SceneTime*>(domainObject);
			if (time->name() == timeName) {
				newTime = time;
				break;
			}
		}

		//
		// Если такого времени ещё нет, то сохраним его
		//
		if (!DomainObject::isValid(newTime)) {
            newTime = new SceneTime(Identifier(), timeName);

			//
			// ... в базе данных
			//
			MapperFacade::timeMapper()->insert(newTime);

			//
			// ... в списке
			//
			all()->append(newTime);
		}
	}

	return newTime;
}

void TimeStorage::removeTime(const QString& _name)
{
    for (DomainObject* domainObject : all()->toList()) {
        SceneTime* time = dynamic_cast<SceneTime*>(domainObject);
        if (time->equal(_name)) {
            MapperFacade::timeMapper()->remove(time);
            all()->remove(time);
            break;
        }
    }
}

void TimeStorage::clear()
{
	delete m_all;
	m_all = 0;

	MapperFacade::timeMapper()->clear();
}

void TimeStorage::refresh()
{
	MapperFacade::timeMapper()->refresh(all());
}

TimeStorage::TimeStorage() :
	m_all(0)
{
}
