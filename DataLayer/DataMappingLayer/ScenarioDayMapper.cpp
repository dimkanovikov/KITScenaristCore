#include "ScenarioDayMapper.h"

#include <Domain/ScenarioDay.h>

using namespace DataMappingLayer;


namespace {
	const QString kColumns = " id, name ";
	const QString kTableName = " scenary_days ";
}

ScenarioDay* ScenarioDayMapper::find(const Identifier& _id)
{
	return dynamic_cast<ScenarioDay*>(abstractFind(_id));
}

ScenarioDaysTable* ScenarioDayMapper::findAll()
{
	return qobject_cast<ScenarioDaysTable*>(abstractFindAll());
}

void ScenarioDayMapper::insert(ScenarioDay* _scenarioDay)
{
    abstractInsert(_scenarioDay);
}

void ScenarioDayMapper::update(ScenarioDay* _scenarioDay)
{
    abstractUpdate(_scenarioDay);
}

void ScenarioDayMapper::remove(ScenarioDay* _scenarioDay)
{
    abstractDelete(_scenarioDay);
}

QString ScenarioDayMapper::findStatement(const Identifier& _id) const
{
	QString findStatement =
			QString("SELECT " + kColumns +
					" FROM " + kTableName +
					" WHERE id = %1 "
					)
			.arg(_id.value());
	return findStatement;
}

QString ScenarioDayMapper::findAllStatement() const
{
	return "SELECT " + kColumns + " FROM  " + kTableName;
}

QString ScenarioDayMapper::insertStatement(DomainObject* _subject, QVariantList& _insertValues) const
{
	QString insertStatement =
			QString("INSERT INTO " + kTableName +
					" (id, name) "
					" VALUES(?, ?) "
					);

	ScenarioDay* scenaryDay = dynamic_cast<ScenarioDay*>(_subject );
	_insertValues.clear();
	_insertValues.append(scenaryDay->id().value());
	_insertValues.append(scenaryDay->name());

	return insertStatement;
}

QString ScenarioDayMapper::updateStatement(DomainObject* _subject, QVariantList& _updateValues) const
{
	QString updateStatement =
			QString("UPDATE " + kTableName +
					" SET name = ? "
					" WHERE id = ? "
					);

	ScenarioDay* scenaryDay = dynamic_cast<ScenarioDay*>(_subject);
	_updateValues.clear();
	_updateValues.append(scenaryDay->name());
	_updateValues.append(scenaryDay->id().value());

	return updateStatement;
}

QString ScenarioDayMapper::deleteStatement(DomainObject* _subject, QVariantList& _deleteValues) const
{
	QString deleteStatement = "DELETE FROM " + kTableName + " WHERE id = ?";

	_deleteValues.clear();
	_deleteValues.append(_subject->id().value());

	return deleteStatement;
}

DomainObject* ScenarioDayMapper::doLoad(const Identifier& _id, const QSqlRecord& _record)
{
	const QString name = _record.value("name").toString();

	return new ScenarioDay(_id, name);
}

void ScenarioDayMapper::doLoad(DomainObject* _domainObject, const QSqlRecord& _record)
{
	if (ScenarioDay* day = dynamic_cast<ScenarioDay*>(_domainObject)) {
		const QString name = _record.value("name").toString();
		day->setName(name);
	}
}

DomainObjectsItemModel* ScenarioDayMapper::modelInstance()
{
	return new ScenarioDaysTable;
}

ScenarioDayMapper::ScenarioDayMapper()
{
}
