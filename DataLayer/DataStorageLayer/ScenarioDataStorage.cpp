#include "ScenarioDataStorage.h"

#include <DataLayer/DataMappingLayer/MapperFacade.h>
#include <DataLayer/DataMappingLayer/ScenarioDataMapper.h>

#include <Domain/ScenarioData.h>

using DataStorageLayer::ScenarioDataStorage;
using namespace DataMappingLayer;


void ScenarioDataStorage::clear()
{
    delete m_all;
    m_all = 0;

    MapperFacade::scenarioDataMapper()->clear();
}

void ScenarioDataStorage::refresh()
{
    MapperFacade::scenarioDataMapper()->refresh(all());
}

QString ScenarioDataStorage::name() const
{
    return data(ScenarioData::NAME_KEY)->value();
}

void ScenarioDataStorage::setName(const QString& _name)
{
    saveData(ScenarioData::NAME_KEY, _name);
}

QString ScenarioDataStorage::header() const
{
    return data(ScenarioData::HEADER_KEY)->value();
}

void ScenarioDataStorage::setHeader(const QString& _header)
{
    saveData(ScenarioData::HEADER_KEY, _header);
}

QString ScenarioDataStorage::footer() const
{
    return data(ScenarioData::FOOTER_KEY)->value();
}

void ScenarioDataStorage::setFooter(const QString& _footer)
{
    saveData(ScenarioData::FOOTER_KEY, _footer);
}

QString ScenarioDataStorage::sceneNumbersPrefix() const
{
    return data(ScenarioData::SCENE_NUMBERS_PREFIX_KEY)->value();
}

void ScenarioDataStorage::setSceneNumbersPrefix(const QString& _prefix)
{
    saveData(ScenarioData::SCENE_NUMBERS_PREFIX_KEY, _prefix);
}

QString ScenarioDataStorage::sceneStartNumber() const
{
    return data(ScenarioData::SCENE_START_NUMBER_KEY)->value();
}

void ScenarioDataStorage::setSceneStartNumber(const QString& _startNumber)
{
    saveData(ScenarioData::SCENE_START_NUMBER_KEY, _startNumber);
}

QString ScenarioDataStorage::logline() const
{
    return data(ScenarioData::LOGLINE_KEY)->value();
}

void ScenarioDataStorage::setLogline(const QString& _logline)
{
    saveData(ScenarioData::LOGLINE_KEY, _logline);
}

QString ScenarioDataStorage::additionalInfo() const
{
    return data(ScenarioData::ADDITIONAL_INFO_KEY)->value();
}

void ScenarioDataStorage::setAdditionalInfo(const QString& _additionalInfo)
{
    saveData(ScenarioData::ADDITIONAL_INFO_KEY, _additionalInfo);
}

QString ScenarioDataStorage::genre() const
{
    return data(ScenarioData::GENRE_KEY)->value();
}

void ScenarioDataStorage::setGenre(const QString& _genre)
{
    saveData(ScenarioData::GENRE_KEY, _genre);
}

QString ScenarioDataStorage::author() const
{
    return data(ScenarioData::AUTHOR_KEY)->value();
}

void ScenarioDataStorage::setAuthor(const QString _author)
{
    saveData(ScenarioData::AUTHOR_KEY, _author);
}

QString ScenarioDataStorage::contacts() const
{
    return data(ScenarioData::CONTACTS_KEY)->value();
}

void ScenarioDataStorage::setContacts(const QString& _contacts)
{
    saveData(ScenarioData::CONTACTS_KEY, _contacts);
}

QString ScenarioDataStorage::year() const
{
    return data(ScenarioData::YEAR_KEY)->value();
}

void ScenarioDataStorage::setYear(const QString& _year)
{
    saveData(ScenarioData::YEAR_KEY, _year);
}

QString ScenarioDataStorage::synopsis() const
{
    return data(ScenarioData::SYNOPSIS_KEY)->value();
}

void ScenarioDataStorage::setSynopsis(const QString& _synopsis)
{
    saveData(ScenarioData::SYNOPSIS_KEY, _synopsis);
}

ScenarioDataTable* ScenarioDataStorage::all() const
{
    if (m_all == nullptr) {
        m_all = MapperFacade::scenarioDataMapper()->findAll();
    }
    return m_all;
}

ScenarioData* ScenarioDataStorage::data(const QString& _name) const
{
    //
    // Ищем среди загруженных
    //
    ScenarioData* data = all()->scenarioData(_name);

    //
    // Если не нашли
    //
    if (data == nullptr) {
        //
        // ... создаём новый
        //
        data = new ScenarioData(Identifier(), _name, QString());
        //
        // ... сохраняем
        //
        MapperFacade::scenarioDataMapper()->insert(data);
        //
        // ... и запоминаем локально
        //
        all()->append(data);
    }

    return data;
}

void ScenarioDataStorage::saveData(const QString& _name, const QString& _newValue)
{
    ScenarioData* dataForUpdate = data(_name);
    if (dataForUpdate->value() != _newValue) {
        dataForUpdate->setValue(_newValue);
        MapperFacade::scenarioDataMapper()->update(dataForUpdate);
    }
}

ScenarioDataStorage::ScenarioDataStorage()
{
}

