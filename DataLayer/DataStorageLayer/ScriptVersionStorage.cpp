#include "ScriptVersionStorage.h"

#include <DataLayer/DataMappingLayer/MapperFacade.h>
#include <DataLayer/DataMappingLayer/ScriptVersionMapper.h>

#include <Domain/ScriptVersion.h>

#include <3rd_party/Helpers/TextEditHelper.h>

using namespace DataStorageLayer;
using namespace DataMappingLayer;


ScriptVersionsTable* ScriptVersionStorage::all()
{
    if (m_all == nullptr) {
        m_all = MapperFacade::scriptVersionMapper()->findAll();
    }
    return m_all;
}

ScriptVersion*ScriptVersionStorage::storeScriptVersion(const QDateTime& _datetime, const QColor& _color,
                                                       const QString& _name, const QString& _description)
{
    //
    // Создаём новую версию
    //
    ScriptVersion* newScriptVersion = new ScriptVersion(Identifier(), _datetime, _color, _name, _description);

    //
    // И сохраняем её
    //
    // ... в базе данных
    //
    MapperFacade::scriptVersionMapper()->insert(newScriptVersion);
    //
    // ... в списке
    //
    all()->append(newScriptVersion);

    return newScriptVersion;
}

void ScriptVersionStorage::updateScriptVersion(ScriptVersion* _scriptVersion)
{
    //
    // Сохраним изменение в базе данных
    //
    if (MapperFacade::scriptVersionMapper()->update(_scriptVersion)) {
        //
        // Уведомим об обновлении
        //
        int indexRow = all()->toList().indexOf(_scriptVersion);
        QModelIndex updateIndex = all()->index(indexRow, 0, QModelIndex());
        emit all()->dataChanged(updateIndex, updateIndex);
    }
}

void ScriptVersionStorage::removeScriptVersion(ScriptVersion* _scriptVersion)
{
    all()->remove(_scriptVersion);
    MapperFacade::scriptVersionMapper()->remove(_scriptVersion);
}

void ScriptVersionStorage::clear()
{
    delete m_all;
    m_all = nullptr;

    MapperFacade::scriptVersionMapper()->clear();
}

void ScriptVersionStorage::refresh()
{
    MapperFacade::scriptVersionMapper()->refresh(all());
}
