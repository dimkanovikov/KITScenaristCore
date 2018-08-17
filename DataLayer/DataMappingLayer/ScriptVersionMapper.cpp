#include "ScriptVersionMapper.h"

#include <Domain/ScriptVersion.h>

using namespace DataMappingLayer;


namespace {
    const QString kColumns = " id, fk_script_id, username, datetime, color, name, description, script_text ";
    const QString kTableName = " script_versions ";
}

ScriptVersion* ScriptVersionMapper::find(const Identifier& _id)
{
    return dynamic_cast<ScriptVersion*>(abstractFind(_id));
}

ScriptVersionsTable* ScriptVersionMapper::findAll()
{
    return qobject_cast<ScriptVersionsTable*>(abstractFindAll());
}

void ScriptVersionMapper::insert(ScriptVersion* _scriptVersion)
{
    abstractInsert(_scriptVersion);
}

bool ScriptVersionMapper::update(ScriptVersion* _scriptVersion)
{
    return abstractUpdate(_scriptVersion);
}

void ScriptVersionMapper::remove(ScriptVersion* _scriptVersion)
{
    abstractDelete(_scriptVersion);
}

QString ScriptVersionMapper::findStatement(const Identifier& _id) const
{
    QString findStatement =
            QString("SELECT " + kColumns +
                    " FROM " + kTableName +
                    " WHERE id = %1 "
                    )
            .arg(_id.value());
    return findStatement;
}

QString ScriptVersionMapper::findAllStatement() const
{
    return "SELECT " + kColumns + " FROM  " + kTableName + " ORDER BY id ";
}

QString ScriptVersionMapper::insertStatement(DomainObject* _subject, QVariantList& _insertValues) const
{
    QString insertStatement =
            QString("INSERT INTO " + kTableName +
                    " (id, fk_script_id, username, datetime, color, name, description, script_text) "
                    " VALUES(?, ?, ?, ?, ?, ?, ?, ?) "
                    );

    ScriptVersion* scriptVersion = dynamic_cast<ScriptVersion*>(_subject );
    _insertValues.clear();
    _insertValues.append(scriptVersion->id().value());
    _insertValues.append(1);
    _insertValues.append(scriptVersion->username());
    _insertValues.append(scriptVersion->datetime().toString("yyyy-MM-dd hh:mm:ss:zzz"));
    _insertValues.append(scriptVersion->color().name());
    _insertValues.append(scriptVersion->name());
    _insertValues.append(scriptVersion->description());
    _insertValues.append(scriptVersion->scriptText());

    return insertStatement;
}

QString ScriptVersionMapper::updateStatement(DomainObject* _subject, QVariantList& _updateValues) const
{
    QString updateStatement =
            QString("UPDATE " + kTableName +
                    " SET username = ?, "
                    " datetime = ?, "
                    " color = ?, "
                    " name = ?, "
                    " description = ?, "
                    " script_text = ? "
                    " WHERE id = ? "
                    );

    ScriptVersion* scriptVersion = dynamic_cast<ScriptVersion*>(_subject);
    _updateValues.clear();
    _updateValues.append(scriptVersion->username());
    _updateValues.append(scriptVersion->datetime().toString("yyyy-MM-dd hh:mm:ss:zzz"));
    _updateValues.append(scriptVersion->color().name());
    _updateValues.append(scriptVersion->name());
    _updateValues.append(scriptVersion->description());
    _updateValues.append(scriptVersion->scriptText());
    _updateValues.append(scriptVersion->id().value());

    return updateStatement;
}

QString ScriptVersionMapper::deleteStatement(DomainObject* _subject, QVariantList& _deleteValues) const
{
    QString deleteStatement = "DELETE FROM " + kTableName + " WHERE id = ?";

    _deleteValues.clear();
    _deleteValues.append(_subject->id().value());

    return deleteStatement;
}

DomainObject* ScriptVersionMapper::doLoad(const Identifier& _id, const QSqlRecord& _record)
{
    const QString username = _record.value("username").toString();
    const QDateTime datetime = QDateTime::fromString(_record.value("datetime").toString(), "yyyy-MM-dd hh:mm:ss:zzz");
    const QColor color = QColor(_record.value("color").toString());
    const QString name = _record.value("name").toString();
    const QString description = _record.value("description").toString();
    const QString scriptText = _record.value("script_text").toString();

    return new ScriptVersion(_id, username, datetime, color, name, description, scriptText);
}

void ScriptVersionMapper::doLoad(DomainObject* _domainObject, const QSqlRecord& _record)
{
    if (ScriptVersion* scriptVersion = dynamic_cast<ScriptVersion*>(_domainObject)) {
        const QString username = _record.value("username").toString();
        scriptVersion->setUsername(username);

        const QDateTime datetime = QDateTime::fromString(_record.value("datetime").toString(), "yyyy-MM-dd hh:mm:ss:zzz");
        scriptVersion->setDatetime(datetime);

        const QColor color = QColor(_record.value("color").toString());
        scriptVersion->setColor(color);

        const QString name = _record.value("name").toString();
        scriptVersion->setName(name);

        const QString description = _record.value("description").toString();
        scriptVersion->setDescription(description);

        const QString scriptText = _record.value("script_text").toString();
        scriptVersion->setName(scriptText);
    }
}

DomainObjectsItemModel* ScriptVersionMapper::modelInstance()
{
    return new ScriptVersionsTable;
}

ScriptVersionMapper::ScriptVersionMapper()
{
}
