#ifndef ABSTRACTIMPORTER_H
#define ABSTRACTIMPORTER_H

#include <QApplication>
#include <QColor>
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
        virtual QString importScript(const ImportParameters& _importParameters) const = 0;

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
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "All supported files") + QLatin1String(" (*.dps *.fdx *.trelby *.docx *.doc *.odt *.fountain *.celtx)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Digipitch Screenwriter project") + QLatin1String(" (*.dps)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Final Draft screenplay") + QLatin1String(" (*.fdx)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Final Draft template") + QLatin1String(" (*.fdxt)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Trelby screenplay") + QLatin1String(" (*.trelby)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Office Open XML") + QLatin1String(" (*.docx *.doc)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","OpenDocument text") + QLatin1String(" (*.odt)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Fountain text") + QLatin1String(" (*.fountain)"));
            filters.append(";;");
            filters.append(QApplication::translate("BusinessLogic::AbstractImporter","Celtx project") + QLatin1String(" (*.celtx)"));

            return filters;
        }

        /**
         * @brief Структура для работы с форматирование блоков
         */
        struct TextFormat {
            /**
             * @brief Начальная позиция форматирования
             */
            int start = 0;

            /**
             * @brief Длина форматирования
             */
            int length = 0;

            /**
             * @brief Полужирное
             */
            bool bold = false;

            /**
             * @brief Курсив
             */
            bool italic = false;

            /**
             * @brief Подчёркивание
             */
            bool underline = false;

            /**
             * @brief Задано ли форматирование
             */
            bool isValid() const {
                return
                        bold != false
                        || italic != false
                        || underline != false;
            }

            /**
             * @brief Очистить форматирование
             */
            void clear() {
                bold = false;
                italic = false;
                underline = false;
            }
        };

        /**
         * @brief Структура для работы с заметками на полях
         */
        struct TextNote {
            /**
             * @brief Относится ли заметка ко всему блоку, или только к части, ограниченной start и length
             */
            bool isFullBlock = false;

            /**
             * @brief Начальная позиция форматирования
             */
            int start = 0;

            /**
             * @brief Длина форматирования
             */
            int length = 0;

            /**
             * @brief Текст заметки
             */
            QString comment;

            /**
             * @brief Цвет заметки
             */
            QColor color;

            /**
             * @brief Задана ли заметка
             */
            bool isValid() const {
                return
                        !comment.isEmpty()
                        && color.isValid();
            }

            /**
             * @brief Очистить заметку
             */
            void clear() {
                comment.clear();
                color = QColor();
            }
        };
    };
}

#endif // ABSTRACTIMPORTER_H
