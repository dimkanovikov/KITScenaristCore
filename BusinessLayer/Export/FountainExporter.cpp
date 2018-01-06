#include "FountainExporter.h"

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockInfo.h>

#include <QApplication>
#include <QFile>
#include <QTextBlock>

using namespace BusinessLogic;

namespace {
const QStringList sceneHeadingStart = {QApplication::translate("BusinessLayer::FountainExporter", "INT"),
                                       QApplication::translate("BusinessLayer::FountainExporter", "EXT"),
                                       QApplication::translate("BusinessLayer::FountainExporter", "EST"),
                                       QApplication::translate("BusinessLayer::FountainExporter", "INT./EXT"),
                                       QApplication::translate("BusinessLayer::FountainExporter", "INT/EXT")};
}

FountainExporter::FountainExporter() :
    AbstractExporter()
{
}

void FountainExporter::exportTo(ScenarioDocument *_scenario, const ExportParameters &_exportParameters) const
{
    //
    // Открываем документ на запись
    //
    QFile fountainFile(_exportParameters.filePath);
    if (fountainFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        //
        // При необходимости пишем титульную страницу
        //
        if (_exportParameters.printTilte) {
            auto writeLine = [&fountainFile] (const QString& _key, const QString& _value) {
                if (!_key.isEmpty() && !_value.isEmpty()) {
                    fountainFile.write(QString("%1: %2\n").arg(_key).arg(_value).toUtf8());
                }
            };
            auto writeLines = [&fountainFile] (const QString& _key, const QString& _value) {
                if (!_key.isEmpty() && !_value.isEmpty()) {
                    fountainFile.write(QString("%1:\n").arg(_key).toUtf8());
                    for (const QString& line : _value.split("\n")) {
                        fountainFile.write(QString("   %1\n").arg(line).toUtf8());
                    }
                }
            };
            writeLine("Title", _exportParameters.scriptName);
            writeLine("Credit", _exportParameters.scriptGenre);
            writeLine("Author", _exportParameters.scriptAuthor);
            writeLine("Source", _exportParameters.scriptAdditionalInfo);
            writeLine("Draft date", _exportParameters.scriptYear);
            writeLines("Contact", _exportParameters.scriptContacts);
            //
            // Пустая строка в конце
            //
            fountainFile.write("\n");
        }


        //
        // Нам нужен не просто QTextDocument, а тот,
        // в котором будут еще редакторские заметки, Noprintable и директории
        //
        ExportParameters fakeParameters;
        fakeParameters.isOutline = true;
        fakeParameters.isScript = _exportParameters.isScript;
        fakeParameters.saveReviewMarks = true;
        fakeParameters.printInvisible = true;

        //
        // Импорт фонтана сам установит номера сцен при необходимости
        //
        fakeParameters.printScenesNumbers = false;

        QTextDocument* preparedDocument = prepareDocument(_scenario, fakeParameters);
        QTextCursor documentCursor(preparedDocument);

        //
        // Является ли текущий блок первым
        //
        bool isFirst = true;

        //
        // Текущая глубина вложенности директорий
        //
        unsigned dirNesting = 0;

        //
        // Тип предыдущего блока
        //
        ScenarioBlockStyle::Type prevType = ScenarioBlockStyle::Undefined;

        while (!documentCursor.atEnd()) {
            QString paragraphText;
            if (!documentCursor.block().text().isEmpty()) {
                paragraphText = documentCursor.block().text();
                QVector<QTextLayout::FormatRange> notes;

                //
                // Извлечем список редакторских заметок
                //

                //
                // Не знаю, какая это магия, но если вместо этого цикла использовать remove_copy_if
                // или copy_if, то получаем сегфолт
                //
                for (const QTextLayout::FormatRange& format : documentCursor.block().textFormats()) {
                    if (!format.format.property(ScenarioBlockStyle::PropertyComments).toStringList().isEmpty()) {
                        notes.push_back(format);
                    }
                }

                //
                // Если всего одна редакторская заметка на весь текст, то расположим ее после блока на отдельной строке
                // (делается не здесь, а в конце цикла), а иначе просто вставим в блок заметки
                //
                bool fullBlockComment = true;
                if (notes.size() != 1
                        || notes.front().length != paragraphText.size()) {
                    fullBlockComment = false;

                    //
                    // Обрабатывать редакторские заметки надо с конца, чтобы не сбилась их позиция вставки
                    //
                    for (int i = notes.size() - 1; i >= 0; --i) {
                        //
                        // Извлечем список редакторских заметок для данной области блока
                        //
                        const QStringList comments = notes[i].format.property(ScenarioBlockStyle::PropertyComments)
                                .toStringList();
                        //
                        // Вставлять редакторские заметки нужно с конца, чтобы не сбилась их позиция вставки
                        //
                        for (int j = comments.size() - 1; j >= 0; --j) {
                            paragraphText.insert(notes[i].start + notes[i].length, "[[" + comments[j] + "]]");
                        }
                    }
                }

                //
                // Пропустить запись текущего блока
                //
                bool skipBlock = false;

                switch (ScenarioBlockStyle::forBlock(documentCursor.block())) {
                    case ScenarioBlockStyle::SceneHeading:
                    {
                        //
                        // Если заголовок сцены начинается с одного из ключевых слов, то все хорошо
                        //
                        bool startsWithHeading = false;
                        for (const QString& heading : sceneHeadingStart) {
                            if (paragraphText.startsWith(heading)) {
                                startsWithHeading = true;
                                break;
                            }
                        }

                        //
                        // Иначе, нужно сказать, что это заголовок сцены добавлением точки в начало
                        //
                        if (!startsWithHeading) {
                            paragraphText.prepend('.');
                        }

                        //
                        // А если печатаем номера сцен, то добавим в конец этот номер, окруженный #
                        //
                        if (_exportParameters.printScenesNumbers) {
                            QTextBlockUserData* textBlockData = documentCursor.block().userData();
                            SceneHeadingBlockInfo* sceneInfo = dynamic_cast<SceneHeadingBlockInfo*>(textBlockData);
                            if (sceneInfo != 0 && sceneInfo->sceneNumber()) {
                                paragraphText += " #" + QString::number(sceneInfo->sceneNumber()) + "#";
                            }
                        }

                        if (!isFirst) {
                            paragraphText.prepend('\n');
                        }
                        break;
                    }

                    case ScenarioBlockStyle::Character:
                    {
                        if (paragraphText != paragraphText.toUpper()) {
                            //
                            // Если название персонажа не состоит из заглавных букв,
                            // то необходимо добавить @ в начало
                            //
                            paragraphText.prepend('@');
                        }
                        paragraphText.prepend('\n');
                        break;
                    }

                    case ScenarioBlockStyle::Transition:
                    {
                        //
                        // Если переход задан заглавными буквами и в конце есть TO:
                        //
                        if (paragraphText.toUpper() == paragraphText
                            && paragraphText.endsWith("TO:")) {
                            //
                            // Ничего делать не надо, всё распознается нормально
                            //
                        }
                        //
                        // А если переход задан как то иначе
                        //
                        else {
                            //
                            // То надо добавить в начало >
                            //
                            paragraphText.prepend("> ");
                        }
                        paragraphText.prepend('\n');
                        break;
                    }

                    case ScenarioBlockStyle::NoprintableText:
                    {
                        //
                        // Обернем в /* и */
                        //
                        paragraphText = "\n/*\n" + paragraphText + "\n*/";
                        break;
                    }

                    case ScenarioBlockStyle::Action:
                    {
                        //
                        // Чтобы действия шли друг за другом более аккуратно,
                        // не будем разделять подряд идущие действия пустой строкой
                        //
                        if (prevType != ScenarioBlockStyle::Action
                            && !isFirst) {
                            paragraphText.prepend('\n');
                        }
                        break;
                    }

                    case ScenarioBlockStyle::SceneDescription:
                    {
                        //
                        // Блоки описания сцены предворяются = и расставляются обособлено
                        //
                        paragraphText.prepend("\n= ");
                        break;
                    }

                    case ScenarioBlockStyle::Lyrics:
                    {
                        //
                        // Добавим ~ вначало блока лирики
                        //
                        paragraphText.prepend("~ ");
                        break;
                    }

                    case ScenarioBlockStyle::FolderHeader:
                    {
                        //
                        // Напечатаем в начале столько #, насколько глубоко мы в директории
                        //
                        ++dirNesting;
                        paragraphText = " " + paragraphText;
                        for (unsigned i = 0; i != dirNesting; ++i) {
                            paragraphText = '#' + paragraphText;
                        }
                        paragraphText.prepend('\n');
                        break;
                    }

                    case ScenarioBlockStyle::FolderFooter:
                    {
                        --dirNesting;
                        skipBlock = true;
                        break;
                    }

                    case ScenarioBlockStyle::Dialogue:
                    case ScenarioBlockStyle::Parenthetical:
                        break;
                    default:
                    {
                        //
                        // Игнорируем неизвестные блоки
                        //
                        skipBlock = true;
                    }
                }
                paragraphText += '\n';

                if (fullBlockComment) {
                    //
                    // А это как раз случай одной большой редакторской заметки
                    //
                    paragraphText += '\n';
                    QStringList comments = notes[0].format
                            .property(ScenarioBlockStyle::PropertyComments).toStringList();
                    for (const QString& comment: comments) {
                        paragraphText += "[[" + comment + "]]\n";
                    }
                }

                prevType = ScenarioBlockStyle::forBlock(documentCursor.block());

                //
                // Запишем получившуюся строку
                //
                if (!skipBlock) {
                    isFirst = false;
                    fountainFile.write(paragraphText.toUtf8());
                }
            }
            documentCursor.movePosition(QTextCursor::EndOfBlock);
            documentCursor.movePosition(QTextCursor::NextBlock);
        }
    }
}

void FountainExporter::exportTo(const ResearchModelCheckableProxy* _researchModel, const ExportParameters& _exportParameters) const
{
    Q_UNUSED(_researchModel);
    Q_UNUSED(_exportParameters);
}
