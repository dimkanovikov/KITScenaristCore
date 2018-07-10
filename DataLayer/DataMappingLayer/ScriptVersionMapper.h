#ifndef SCRIPTVERSIONMAPPER_H
#define SCRIPTVERSIONMAPPER_H

#include "AbstractMapper.h"
#include "MapperFacade.h"

namespace Domain {
    class ScriptVersion;
    class ScriptVersionsTable;
}

using namespace Domain;


namespace DataMappingLayer
{
    class ScriptVersionMapper : public AbstractMapper
    {
    public:
        ScriptVersion* find(const Identifier& _id);
        ScriptVersionsTable* findAll();
        void insert(ScriptVersion* _scriptVersion);
        bool update(ScriptVersion* _scriptVersion);
        void remove(ScriptVersion* _scriptVersion);

    protected:
        QString findStatement(const Identifier& _id) const;
        QString findAllStatement() const;
        QString insertStatement(DomainObject* _subject, QVariantList& _insertValues) const;
        QString updateStatement(DomainObject* _subject, QVariantList& _updateValues) const;
        QString deleteStatement(DomainObject* _subject, QVariantList& _deleteValues) const;

    protected:
        DomainObject* doLoad(const Identifier& _id, const QSqlRecord& _record);
        void doLoad(DomainObject* _domainObject, const QSqlRecord& _record);
        DomainObjectsItemModel* modelInstance();

    private:
        ScriptVersionMapper();

        // Для доступа к конструктору
        friend class MapperFacade;
    };
}

#endif // SCRIPTVERSIONMAPPER_H
