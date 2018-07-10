#include "TransitionMapper.h"

#include <Domain/Transition.h>

using namespace DataMappingLayer;


namespace {
    const QString kColumns = " id, name ";
    const QString kTableName = " transitions ";
}

Transition* TransitionMapper::find(const Identifier& _id)
{
    return dynamic_cast<Transition*>(abstractFind(_id));
}

TransitionsTable* TransitionMapper::findAll()
{
    return qobject_cast<TransitionsTable*>(abstractFindAll());
}

void TransitionMapper::insert(Transition* _transition)
{
    abstractInsert(_transition);
}

void TransitionMapper::update(Transition* _transition)
{
    abstractUpdate(_transition);
}

void TransitionMapper::remove(Transition* _transition)
{
    abstractDelete(_transition);
}

QString TransitionMapper::findStatement(const Identifier& _id) const
{
    QString findStatement =
            QString("SELECT " + kColumns +
                    " FROM " + kTableName +
                    " WHERE id = %1 "
                    )
            .arg(_id.value());
    return findStatement;
}

QString TransitionMapper::findAllStatement() const
{
    return "SELECT " + kColumns + " FROM  " + kTableName + " ORDER BY id ";
}

QString TransitionMapper::insertStatement(DomainObject* _subject, QVariantList& _insertValues) const
{
    QString insertStatement =
            QString("INSERT INTO " + kTableName +
                    " (id, name) "
                    " VALUES(?, ?) "
                    );

    Transition* transition = dynamic_cast<Transition*>(_subject );
    _insertValues.clear();
    _insertValues.append(transition->id().value());
    _insertValues.append(transition->name());

    return insertStatement;
}

QString TransitionMapper::updateStatement(DomainObject* _subject, QVariantList& _updateValues) const
{
    QString updateStatement =
            QString("UPDATE " + kTableName +
                    " SET name = ? "
                    " WHERE id = ? "
                    );

    Transition* transition = dynamic_cast<Transition*>(_subject);
    _updateValues.clear();
    _updateValues.append(transition->name());
    _updateValues.append(transition->id().value());

    return updateStatement;
}

QString TransitionMapper::deleteStatement(DomainObject* _subject, QVariantList& _deleteValues) const
{
    QString deleteStatement = "DELETE FROM " + kTableName + " WHERE id = ?";

    _deleteValues.clear();
    _deleteValues.append(_subject->id().value());

    return deleteStatement;
}

DomainObject* TransitionMapper::doLoad(const Identifier& _id, const QSqlRecord& _record)
{
    const QString name = _record.value("name").toString();

    return new Transition(_id, name);
}

void TransitionMapper::doLoad(DomainObject* _domainObject, const QSqlRecord& _record)
{
    if (Transition* transition = dynamic_cast<Transition*>(_domainObject)) {
        const QString name = _record.value("name").toString();
        transition->setName(name);
    }
}

DomainObjectsItemModel* TransitionMapper::modelInstance()
{
    return new TransitionsTable;
}

TransitionMapper::TransitionMapper()
{
}
