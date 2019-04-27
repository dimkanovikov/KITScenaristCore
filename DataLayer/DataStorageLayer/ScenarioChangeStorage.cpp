#include "ScenarioChangeStorage.h"

#include "SettingsStorage.h"

#include <DataLayer/Database/Database.h>
#include <DataLayer/DataMappingLayer/MapperFacade.h>
#include <DataLayer/DataMappingLayer/ScenarioChangeMapper.h>

#include <Domain/ScenarioChange.h>

#include <3rd_party/Helpers/PasswordStorage.h>

using namespace DataStorageLayer;
using namespace DataMappingLayer;


ScenarioChangesTable* ScenarioChangeStorage::all()
{
    //
    // Не загружаем старые значения, работаем только с новыми,
    // сделанными с момента открытия проекта
    //
    if (m_all == 0) {
        m_all = MapperFacade::scenarioChangeMapper()->findLast(1);
//        m_all = MapperFacade::scenarioChangeMapper()->findAll();
    }
    return m_all;
}

void ScenarioChangeStorage::loadLast(int _size)
{
    if (m_all->size() >= _size) {
        return;
    }

    QScopedPointer<ScenarioChangesTable> lastModel(
            MapperFacade::scenarioChangeMapper()->findLast(_size));
    if (lastModel->size() == 0) {
        return;
    }

    for (int changeIndex = _size - m_all->size() - 1; changeIndex >= 0; --changeIndex) {
        DomainObject* change = lastModel->toList()[changeIndex];
        m_all->prepend(change);
    }
}

ScenarioChange* ScenarioChangeStorage::last()
{
    return all()->last();
}

ScenarioChange* ScenarioChangeStorage::append(const QString& _id, const QString& _datetime,
    const QString& _user, const QString& _undoPatch, const QString& _redoPatch, bool _isDraft)
{
    if (m_uuids.contains({ _id, _datetime })) {
        Q_ASSERT(0);
    }

    //
    // Пробуем загрузить дату из расширенного формата
    //
    QDateTime changeDatetime = QDateTime::fromString(_datetime, "yyyy-MM-dd hh:mm:ss:zzz");
    //
    // ... а если не удалось, загружаем из ограниченного
    //
    if (!changeDatetime.isValid()) {
        changeDatetime = QDateTime::fromString(_datetime, "yyyy-MM-dd hh:mm:ss");
    }
    //
    // ... если даже это не помогло, то устанавливаем текущую
    //
    if (!changeDatetime.isValid()) {
        changeDatetime = QDateTime::currentDateTimeUtc();
    }

    //
    // Формируем изменение
    //
    ScenarioChange* change =
            new ScenarioChange(Identifier(), QUuid(_id), changeDatetime,
                _user, _undoPatch, _redoPatch, _isDraft);
    //
    // Добавляем его в список всех изменений
    //
    all()->append(change);
    //
    // ... и на сохранение
    //
    allToSave()->append(change);

    //
    // Сохраняем идентификатор в списке
    //
    m_uuids.insert({ _id, _datetime });

    //
    // Возвращаем клиенту
    //
    return change;
}

ScenarioChange* ScenarioChangeStorage::append(const QString& _user, const QString& _undoPatch,
    const QString& _redoPatch, bool _isDraft)
{
    return
            append(QUuid::createUuid().toString(), QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss:zzz"),
                   _user, _undoPatch, _redoPatch, _isDraft);
}

void ScenarioChangeStorage::removeLast()
{
    auto lastChange = all()->last();
    m_uuids.remove({ lastChange->uuid().toString(), lastChange->datetime().toString("yyyy-MM-dd hh:mm:ss:zzz") });

    //
    // Если изменение ещё не сохранено, то просто удаляем его из списков
    //
    if (allToSave()->contains(lastChange)) {
        allToSave()->remove(lastChange);
        all()->remove(lastChange);
    }
    //
    // В противном случае удаляем последнее изменение из базы данных
    //
    else {
        MapperFacade::scenarioChangeMapper()->remove(lastChange);
        all()->remove(lastChange);
    }
}

void ScenarioChangeStorage::store()
{
    //
    // Сохраняем все несохранённые изменения
    //
    DatabaseLayer::Database::transaction();
    foreach (DomainObject* domainObject, allToSave()->toList()) {
        ScenarioChange* change = dynamic_cast<ScenarioChange*>(domainObject);
        if (!change->id().isValid()) {
            MapperFacade::scenarioChangeMapper()->insert(change);
        }
    }
    DatabaseLayer::Database::commit();

    //
    // Очищаем список на сохранение
    //
    const bool DONT_REMOVE_ITEMS = false;
    allToSave()->clear(DONT_REMOVE_ITEMS);
}

void ScenarioChangeStorage::clear()
{
    delete m_all;
    m_all = 0;

    delete m_allToSave;
    m_allToSave = 0;

    m_uuids.clear();

    MapperFacade::scenarioChangeMapper()->clear();
}

bool ScenarioChangeStorage::contains(const QString& _uuid, const QString& _datetime)
{
    bool contains = false;

    //
    // Проверяем в новых изменениях ещё не сохранённых в БД
    //
    contains = m_uuids.contains({ _uuid, _datetime });

    //
    // Проверяем в БД
    //
    if (!contains) {
        contains = MapperFacade::scenarioChangeMapper()->contains(_uuid, _datetime);
    }

    return contains;
}

QList<QPair<QString, QString>> ScenarioChangeStorage::uuids() const
{
    return MapperFacade::scenarioChangeMapper()->uuids();
}

QList<QPair<QString, QString>> ScenarioChangeStorage::newUuids(const QString& _fromDatetime)
{
    //
    // Отправляем изменения только от локального пользователя
    //
    const QString username = DataStorageLayer::StorageFacade::userName();

    QList<QPair<QString, QString>> allNew;
    for (DomainObject* domainObject : all()->toList()) {
        ScenarioChange* change = dynamic_cast<ScenarioChange*>(domainObject);
        if (change->user() == username
            && change->datetime().toString("yyyy-MM-dd hh:mm:ss:zzz") >= _fromDatetime) {
            allNew.append({change->uuid().toString(), change->datetime().toString("yyyy-MM-dd hh:mm:ss:zzz")});
        }
    }
    return allNew;
}

ScenarioChange ScenarioChangeStorage::change(const QString& _uuid, const QString& _datetime)
{
    //
    // Если изменение ещё не сохранено, берём его из списке несохранённых
    //
    if (m_uuids.contains({ _uuid, _datetime })) {
        foreach (DomainObject* domainObject, all()->toList()) {
            ScenarioChange* change = dynamic_cast<ScenarioChange*>(domainObject);
            if (change->uuid().toString() == _uuid
                && change->datetime().toString("yyyy-MM-dd hh:mm:ss:zzz") == _datetime) {
                return *change;
            }
        }
    }

    //
    // А если сохранено, то из БД
    //
    return MapperFacade::scenarioChangeMapper()->change(_uuid, _datetime);
}

ScenarioChangesTable* ScenarioChangeStorage::allToSave()
{
    if (m_allToSave == 0) {
        m_allToSave = new ScenarioChangesTable;
    }
    return m_allToSave;
}

ScenarioChangeStorage::ScenarioChangeStorage() :
    m_all(0),
    m_allToSave(0)
{
}
