#ifndef RESEARCHMAPPER_H
#define RESEARCHMAPPER_H

#include "AbstractMapper.h"
#include "MapperFacade.h"

#include <Domain/DomainObject.h>

#include <QCache>

namespace Domain {
    class Research;
    class ResearchTable;
}

using namespace Domain;


namespace DataMappingLayer
{
    /**
     * @brief Класс загрузчика изображений объектов разработки
     */
    class ResearchImageMapper : public Domain::AbstractImageWrapper
    {
    public:
        ResearchImageMapper();

        /**
         * @brief Получить изображение для заданного объекта
         */
        QPixmap image(const DomainObject* _forObject) const override;

        /**
         * @brief Установить изображение для заданного обхекта
         */
        void setImage(const QPixmap& _image, const DomainObject* _forObject) override;

        /**
         * @brief Сохранить новое изображения если оно менялось
         */
        void save(const DomainObject* _forObject) const;

        /**
         * @brief Удалить изображение для заданного объекта
         */
        void remove(const DomainObject* _forObject) const;

    private:
        /**
         * @brief Кэш загруженных изображений
         */
        mutable QCache<int, QPixmap> m_imagesCache;

        /**
         * @brief Список новых изображений
         */
        mutable QMap<int, QPixmap> m_newImages;
    };

    // ****

    /**
     * @brief Отображатель данных о элементах разработки из БД в объекты
     */
    class ResearchMapper : public AbstractMapper
    {
    public:
        Research* find(const Identifier& _id);
        ResearchTable* findAll();
        ResearchTable* findCharacters();
        ResearchTable* findLocations();
        void insert(Research* _place);
        bool update(Research* _place);
        void remove(Research* _place);

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
        ResearchMapper();
        // Для доступа к конструктору
        friend class MapperFacade;

        /**
         * @brief Загрузчик фотографий для объектов разработки
         */
        ResearchImageMapper m_imageWrapper;
    };
}

#endif // RESEARCHMAPPER_H
