#include "TransitionStorage.h"

#include <DataLayer/DataMappingLayer/MapperFacade.h>
#include <DataLayer/DataMappingLayer/TransitionMapper.h>

#include <Domain/Transition.h>

#include <3rd_party/Helpers/TextEditHelper.h>

using namespace DataStorageLayer;
using namespace DataMappingLayer;


TransitionsTable* TransitionStorage::all()
{
    if (m_all == nullptr) {
        m_all = MapperFacade::transitionMapper()->findAll();
    }
    return m_all;
}

Transition* TransitionStorage::storeTransition(const QString& _transitionName)
{
    Transition* newTransition = nullptr;

    const QString transitionName = TextEditHelper::smartToUpper(_transitionName).simplified();

    //
    // Если переход можно сохранить
    //
    if (!transitionName.isEmpty()) {
        //
        // Проверяем наличие данного перехода
        //
        foreach (DomainObject* domainObject, all()->toList()) {
            Transition* transition = dynamic_cast<Transition*>(domainObject);
            if (transition->name() == transitionName) {
                newTransition = transition;
                break;
            }
        }

        //
        // Если такого перехода ещё нет, то сохраним его
        //
        if (!DomainObject::isValid(newTransition)) {
            newTransition = new Transition(Identifier(), transitionName);

            //
            // ... в базе данных
            //
            MapperFacade::transitionMapper()->insert(newTransition);

            //
            // ... в списке
            //
            all()->append(newTransition);
        }
    }

    return newTransition;
}

void TransitionStorage::removeTransition(const QString& _name)
{
    for (DomainObject* domainObject : all()->toList()) {
        Transition* transition = dynamic_cast<Transition*>(domainObject);
        if (transition->equal(_name)) {
            MapperFacade::transitionMapper()->remove(transition);
            all()->remove(transition);
            break;
        }
    }
}

void TransitionStorage::clear()
{
    delete m_all;
    m_all = 0;

    MapperFacade::transitionMapper()->clear();
}

void TransitionStorage::refresh()
{
    MapperFacade::transitionMapper()->refresh(all());
}

TransitionStorage::TransitionStorage()
{
}
