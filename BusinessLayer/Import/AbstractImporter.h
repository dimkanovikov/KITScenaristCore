#ifndef ABSTRACTIMPORTER_H
#define ABSTRACTIMPORTER_H

#include <QApplication>
#include <QString>


namespace BusinessLogic
{
    /**
     * @brief Параметры импорта
     */
    class ImportParameters
    {
    public:
        ImportParameters() :
            outline(false),
            removeScenesNumbers(true),
            insertionMode(ReplaceDocument),
            findCharactersAndLocations(true)
        {}

        /**
         * @brief Режим текста: true - поэпизодник, false - сценарий
         */
        bool outline;

        /**
         * @brief Путь к импортируемому файлу
         */
        QString filePath;

        /**
         * @brief Необходимо ли удалять номера сцен
         */
        bool removeScenesNumbers;

        /**
         * @brief Типы способа вставки импортируемого текста в сценарий
         */
        enum InsertionMode {
            ReplaceDocument,
            ToCursorPosition,
            ToDocumentEnd
        };

        /**
         * @brief Способ вставки импортируемого текста в сценарий
         */
        InsertionMode insertionMode;

        /**
         * @brief Искать ли персонажей и локации
         */
        bool findCharactersAndLocations;

        /**
         * @brief Сохранять редакторские заметки
         */
        bool saveReviewMarks;
    };

    /**
     * @brief Абстракция класса импортирующего сценарий
     */
    class AbstractImporter
    {
    public:
        virtual ~AbstractImporter() {}

        /**
         * @brief Импорт сценария
         * @return Сценарий в xml-формате
         */
        virtual QString importScenario(const ImportParameters& _importParameters) const = 0;

        /**
         * @brief Импорт данных разработки
         * @return Карта элементов разработки
         *
         * Возвращаемая карта состоит из четырёх элементов
         * script - данные о сценарии (QVariantMap)
         * characters - персонажи (QVariantList)
         * locations - локации (QVariantList)
         * documents - документы (QVariantList)
         *
         * Каждый элемент списка является картой (QVariantMap), который содержит параметры
         * специфичные для каждого типа. Тип элемента помещается в поле type и соответствует
         * строке названия типа. Вложенные элементы разработки помещаются в поле childs.
         *
         * Описание полей элементов
         * ---
         * script (QVariantMap)
         * - name (QString)
         * - logline (QString)
         * - genre (QString)
         * - author (QString)
         * - additional_info (QString)
         * - contacts (QString)
         * - year (QString)
         * - title_page (html-представление титульной страницы) (QString)
         * - synopsis (QString)
         * ---
         * type == character (QString)
         * - name (QString)
         * - description (QString)
         * - sort_order (int)
         * - childs (QVariantList)
         * ---
         * type == location (QString)
         * - name (QString)
         * - description (QString)
         * - sort_order (int)
         * - childs (QVariantList)
         * ---
         * type == folder (QString)
         * - name (QString)
         * - description (QString)
         * - sort_order (int)
         * - childs (QVariantList)
         * ---
         * type == text_document (QString)
         * - name (QString)
         * - description (QString)
         * - sort_order (int)
         * ---
         * type == mind_map (QString)
         * - name (QString)
         * - map_content (QString)
         * - sort_order (int)
         * ---
         * type == images_gallery (QString)
         * - name (QString)
         * - sort_order (int)
         * - childs (QVariantList)
         * ---
         * type == image (QString)
         * - name (QString)
         * - data (QByteArray)
         * - sort_order (int)
         * ---
         * type == link (QString)
         * - name (QString)
         * - url (QString)
         * - sort_order (int)
         */
        virtual QVariantMap importResearch(const ImportParameters& _importParameters) const {
            Q_UNUSED(_importParameters);
            return QVariantMap();
        }

        /**
         * @brief Список видов файлов, которые могут быть импортированы
         */
        static QString filters() {
            QString filters;
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "All Supported Files") + QLatin1String(" (*.kitsp *.fdx *.trelby *.docx *.doc *.odt *.fountain)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","KIT Scenarist Project") + QLatin1String(" (*.kitsp)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Final Draft screenplay") + QLatin1String(" (*.fdx)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Final Draft template") + QLatin1String(" (*.fdxt)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Trelby screenplay") + QLatin1String(" (*.trelby)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Office Open XML") + QLatin1String(" (*.docx *.doc)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","OpenDocument Text") + QLatin1String(" (*.odt)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Fountain Text") + QLatin1String(" (*.fountain)"));

            return filters;
        }
    };
}

#endif // ABSTRACTIMPORTER_H
