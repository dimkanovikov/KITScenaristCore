#include "TimeMapper.h"

#include <Domain/SceneTime.h>

using namespace DataMappingLayer;


namespace {
	const QString COLUMNS = " id, name ";
	const QString TABLE_NAME = " times ";
}

SceneTime* TimeMapper::find(const Identifier& _id)
{
    return dynamic_cast<SceneTime*>(abstractFind(_id));
}

SceneTimesTable* TimeMapper::findAll()
{
    return qobject_cast<SceneTimesTable*>(abstractFindAll());
}

void TimeMapper::insert(SceneTime* _time)
{
	abstractInsert(_time);
}

void TimeMapper::update(SceneTime* _time)
{
	abstractUpdate(_time);
}

QString TimeMapper::findStatement(const Identifier& _id) const
{
	QString findStatement =
			QString("SELECT " + COLUMNS +
					" FROM " + TABLE_NAME +
					" WHERE id = %1 "
					)
			.arg(_id.value());
	return findStatement;
}

QString TimeMapper::findAllStatement() const
{
	return "SELECT " + COLUMNS + " FROM  " + TABLE_NAME + " ORDER BY id ";
}

QString TimeMapper::insertStatement(DomainObject* _subject, QVariantList& _insertValues) const
{
	QString insertStatement =
			QString("INSERT INTO " + TABLE_NAME +
					" (id, name) "
					" VALUES(?, ?) "
					);

    SceneTime* time = dynamic_cast<SceneTime*>(_subject );
	_insertValues.clear();
	_insertValues.append(time->id().value());
	_insertValues.append(time->name());

	return insertStatement;
}

QString TimeMapper::updateStatement(DomainObject* _subject, QVariantList& _updateValues) const
{
	QString updateStatement =
			QString("UPDATE " + TABLE_NAME +
					" SET name = ? "
					" WHERE id = ? "
					);

    SceneTime* time = dynamic_cast<SceneTime*>(_subject);
	_updateValues.clear();
	_updateValues.append(time->name());
	_updateValues.append(time->id().value());

	return updateStatement;
}

QString TimeMapper::deleteStatement(DomainObject* _subject, QVariantList& _deleteValues) const
{
	QString deleteStatement = "DELETE FROM " + TABLE_NAME + " WHERE id = ?";

	_deleteValues.clear();
	_deleteValues.append(_subject->id().value());

	return deleteStatement;
}

DomainObject* TimeMapper::doLoad(const Identifier& _id, const QSqlRecord& _record)
{
	const QString name = _record.value("name").toString();

    return new SceneTime(_id, name);
}

void TimeMapper::doLoad(DomainObject* _domainObject, const QSqlRecord& _record)
{
    if (SceneTime* time = dynamic_cast<SceneTime*>(_domainObject)) {
		const QString name = _record.value("name").toString();
		time->setName(name);
	}
}

DomainObjectsItemModel* TimeMapper::modelInstance()
{
    return new SceneTimesTable;
}

TimeMapper::TimeMapper()
{
}
