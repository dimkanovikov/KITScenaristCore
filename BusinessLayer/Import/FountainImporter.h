#ifndef FOUNTAINIMPORTER_H
#define FOUNTAINIMPORTER_H

#include "AbstractImporter.h"
#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

class QXmlStreamWriter;

namespace BusinessLogic
{
    /**
     * @brief Импортер Fountain-документов
     */
    class FountainImporter : public AbstractImporter
    {
        /*
                      . .
                     ` ' `
                 .'''. ' .'''.
                   .. ' ' ..
                  '  '.'.'  '
                  .'''.'.'''
                 ' .''.'.''. '
                   . . : . .
              {} _'___':'___'_ {}
              ||(_____________)||
              """"""(     )""""""
                    _)   (_             .^-^.  ~""~
                   (_______)~~"""~~     '._.'
               ~""~                     .' '.
                                        '.,.'
                                           `'`'
         */
    public:
        FountainImporter();

        /**
         * @brief Импорт сценария из документа
         */
        QString importScenario(const ImportParameters &_importParameters) const override;

        /**
         * @brief Импорт данных разработки
         */
        QVariantMap importResearch(const ImportParameters &_importParameters) const override;

    private:
        /**
         * @brief Обработка конкретного блока перед его добавлением
         */
        void processBlock(QXmlStreamWriter &writer, QString paragraphText,
                        ScenarioBlockStyle::Type blockStyle) const;

        /**
         * @brief Добавление блока
         */
        void appendBlock(QXmlStreamWriter &writer, const QString& paragraphText,
                              QVector<TextFormat>& _formats, ScenarioBlockStyle::Type blockStyle) const;

        /**
         * @brief Добавление комментариев к блоку
         */
        void appendComments(QXmlStreamWriter &writer) const;

        /**
         * @brief Убрать форматирование
         */
        QString simplify(const QString &_value) const;

        /**
         * @brief Добавить форматирование
         */
        void addFormat(QVector<TextFormat>& formats, QVector<TextFormat>& tmpFormats,
                       bool isItalics, bool isBold, bool isUnderline,
                       bool byOne = false) const ;

        //
        // Чтобы не передавать большое число параметров в функции, используются члены класса
        //

        /**
         * @brief Начало позиции в блоке для потенциальной будущей редакторской заметки
         */
        mutable unsigned noteStartPos;

        /**
         * @brief Длина потенциальной будущей редакторской заметки
         */
        mutable unsigned noteLen;

        /**
         * @brief Идет ли сейчас редакторская заметка
         */
        mutable bool notation = false;

        /**
         * @brief Идет ли сейчас комментарий
         */
        mutable bool commenting = false;

        /**
         * @brief Является ли текущий блок первым
         */
        mutable bool firstBlock = true;

        /**
         * @brief Текст блока (на случай multiline комментариев)
         */
        mutable QString text;

        /**
         * @brief Текст редакторской заметки (на случай multiline)
         */
        mutable QString note;

        /**
         * @brief Редакторская заметка к текущему блоку
         * 		  tuple содержит комментарий, позиция и длина области редакторской заметки
         */
        mutable QVector<std::tuple<QString, unsigned, unsigned> > notes;
    };
}

#endif // FOUNTAINIMPORTER_H
