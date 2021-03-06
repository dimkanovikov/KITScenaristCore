#include "ScenarioDayStorage.h"

#include <DataLayer/DataMappingLayer/MapperFacade.h>
#include <DataLayer/DataMappingLayer/ScenarioDayMapper.h>

#include <Domain/ScenarioDay.h>

#include <3rd_party/Helpers/TextEditHelper.h>

using namespace DataStorageLayer;
using namespace DataMappingLayer;


ScenarioDaysTable* ScenarioDayStorage::all()
{
	if (m_all == 0) {
		m_all = MapperFacade::scenarioDayMapper()->findAll();
	}
	return m_all;
}

ScenarioDay* ScenarioDayStorage::storeScenarioDay(const QString& _scenarioDayName)
{
	ScenarioDay* newScenarioDay = 0;

    const QString scenarioDayName = TextEditHelper::smartToUpper(_scenarioDayName).simplified();

	//
	// Если сценарный день можно сохранить
	//
	if (!scenarioDayName.isEmpty()) {
		//
		// Проверяем наличии данного сценарного дня
		//
		foreach (DomainObject* domainObject, all()->toList()) {
			ScenarioDay* scenarioDay = dynamic_cast<ScenarioDay*>(domainObject);
			if (scenarioDay->name() == scenarioDayName) {
				newScenarioDay = scenarioDay;
				break;
			}
		}

		//
		// Если такого сценарного дня ещё нет, то сохраним его
		//
		if (!DomainObject::isValid(newScenarioDay)) {
			newScenarioDay = new ScenarioDay(Identifier(), scenarioDayName);

			//
			// ... в базе данных
			//
			MapperFacade::scenarioDayMapper()->insert(newScenarioDay);

			//
			// ... в списке
			//
			all()->append(newScenarioDay);
		}
	}

	return newScenarioDay;
}

void ScenarioDayStorage::removeScenarioDay(const QString& _name)
{
    for (DomainObject* domainObject : all()->toList()) {
        ScenarioDay* scenarioDay = dynamic_cast<ScenarioDay*>(domainObject);
        if (scenarioDay->equal(_name)) {
            MapperFacade::scenarioDayMapper()->remove(scenarioDay);
            all()->remove(scenarioDay);
            break;
        }
    }
}

void ScenarioDayStorage::clear()
{
	delete m_all;
	m_all = 0;

	MapperFacade::scenarioDayMapper()->clear();
}

void ScenarioDayStorage::refresh()
{
	MapperFacade::scenarioDayMapper()->refresh(all());
}

ScenarioDayStorage::ScenarioDayStorage() :
	m_all(0)
{
}
