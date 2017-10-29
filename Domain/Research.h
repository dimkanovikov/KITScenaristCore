#ifndef RESEARCH_H
#define RESEARCH_H

#include "DomainObject.h"

#include <QPixmap>
#include <QString>


namespace Domain
{
    /**
     * @brief Класс разработки
     */
    class Research : public DomainObject
    {
    public:
        /**
         * @brief Типы разработок
         */
        enum Type {
            Scenario,
            TitlePage,
            Synopsis,
            ResearchRoot,
            Folder,
            Text,
            Url,
            ImagesGallery,
            Image,
            MindMap,
            CharactersRoot = 100, // оставляем резервное место для новых элементов разработки
            Character,
            LocationsRoot = 200,
            Location
        };

    public:
        virtual ~Research() = default;

        /**
         * @brief Получить родителя
         */
        Research* parent() const;

        /**
         * @brief Установить родителя
         */
        void setParent(Research* _parent);

        /**
         * @brief Получить тип
         */
        Type type() const;

        /**
         * @brief Установить тип
         */
        void setType(Type _type);

        /**
         * @brief Получить название
         */
        QString name() const;

        /**
         * @brief Установить название
         */
        void setName(const QString& _name);

        /**
         * @brief Получить описание
         */
        QString description() const;

        /**
         * @brief Установить описание
         */
        void setDescription(const QString& _description);

        /**
         * @brief Добавить описание к текущему
         */
        virtual void addDescription(const QString& _description);

        /**
         * @brief Получить ссылку
         */
        QString url() const;

        /**
         * @brief Установить ссылку
         */
        void setUrl(const QString& _url);

        /**
         * @brief Получить изображение
         */
        QPixmap image() const;

        /**
         * @brief Установить изображение
         */
        void setImage(const QPixmap& _image);

        /**
         * @brief Получить позицию сортировки
         */
        int sortOrder() const;

        /**
         * @brief Установить позицию сортировки
         */
        void setSortOrder(int _sortOrder);

        /**
         * @brief Установить загрузчик изображений
         */
        void setImageWrapper(AbstractImageWrapper* _imageWrapper);

    protected:
        Research(const Identifier& _id, Research* _parent, Type _type, int _sortOrder,
            const QString& _name, const QString& _description = QString(),
            const QString& _url = QString(), AbstractImageWrapper* _imageWrapper = nullptr);
        friend class ResearchBuilder;

    private:
        /**
         * @brief Родительский элемент
         */
        Research* m_parent;

        /**
         * @brief Тип
         */
        Type m_type;

        /**
         * @brief Название
         */
        QString m_name;

        /**
         * @brief Описание
         * @note Html-форматированный текст. Если в элементе хранится ссылка на интернет-ресурс,
         *		 то в этом поле кэшируется содержимое интернет-страницы
         */
        QString m_description;

        /**
         * @brief Ссылка на интернет-ресурс
         */
        QString m_url;

        /**
         * @brief Порядок сортировки
         */
        int m_sortOrder;

        /**
         * @brief Загрузчик фотографий
         */
        AbstractImageWrapper* m_image = nullptr;
    };

    /**
     * @brief Класс персонажа из разработки
     */
    class ResearchCharacter : public Research
    {
    public:
        /**
         * @brief Получить настоящее имя персонажа
         */
        QString realName() const;

        /**
         * @brief Установить настоящее имя персонажа
         */
        void setRealName(const QString& _name);

        /**
         * @brief Получить описательную часть о персонаже
         */
        QString descriptionText() const;

        /**
         * @brief Установить описательную часть
         */
        void setDescriptionText(const QString& _description);

        /**
         * @brief Добавить описание к текущему
         */
        void addDescription(const QString& _description) override;

    protected:
        ResearchCharacter(const Identifier& _id, Research* _parent, Type _type, int _sortOrder,
            const QString& _name, const QString& _description = QString(),
            const QString& _url = QString(), AbstractImageWrapper* _imageWrapper = nullptr);
        friend class ResearchBuilder;
    };

    /**
     * @brief Фабрика для создания элементов разработки
     */
    class ResearchBuilder
    {
    public:
        /**
         * @brief Создать элемент разработки
         */
        static Research* create(const Identifier& _id, Research* _parent, Research::Type _type,
            int _sortOrder, const QString& _name, const QString& _description = QString(),
            const QString& _url = QString(), AbstractImageWrapper* _imageWrapper = nullptr);
    };

    // ****

    class ResearchTable : public DomainObjectsItemModel
    {
        Q_OBJECT

    public:
        explicit ResearchTable(QObject* _parent = 0);

    public:
        enum Column {
            Undefined,
            Id,
            Name
        };

    public:
        int columnCount(const QModelIndex&) const;
        QVariant data(const QModelIndex& _index, int _role) const;

    private:
        Column sectionToColumn(int _section) const;
    };
}

#endif // RESEARCH_H
