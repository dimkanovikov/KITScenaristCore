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
        QString importScript(const ImportParameters& _importParameters) const override;

        /**
         * @brief Импорт данных разработки
         */
        QVariantMap importResearch(const ImportParameters& _importParameters) const override;

    private:
        /**
         * @brief Обработка конкретного блока перед его добавлением
         */
        void processBlock(const QString& _paragraphText, ScenarioBlockStyle::Type _blockStyle,
                          QXmlStreamWriter& _writer) const;

        /**
         * @brief Добавление блока
         */
        void appendBlock(const QString& _paragraphText, ScenarioBlockStyle::Type _blockStyle,
                         QXmlStreamWriter& _writer) const;

        /**
         * @brief Добавление комментариев к блоку
         */
        void appendComments(QXmlStreamWriter& _writer) const;

        /**
         * @brief Убрать форматирование
         */
        QString simplify(const QString& _value) const;

        /**
         * @brief Добавить форматирование
         * @param _atCurrentCharacter - начинается/заканчивается ли форматирование
         *        в текущей позиции (true), или захватывает последний символ (false)
         */
        void processFormat(bool _italics, bool _bold, bool _underline,
                       bool _forCurrentCharacter = false) const ;

        //
        // Чтобы не передавать большое число параметров в функции, используются члены класса
        //

        /**
         * @brief Начало позиции в блоке для потенциальной будущей редакторской заметки
         */
        mutable unsigned m_noteStartPos = 0;

        /**
         * @brief Длина потенциальной будущей редакторской заметки
         */
        mutable unsigned m_noteLen = 0;

        /**
         * @brief Идет ли сейчас редакторская заметка
         */
        mutable bool m_isNotation = false;

        /**
         * @brief Идет ли сейчас комментарий
         */
        mutable bool m_isCommenting = false;

        /**
         * @brief Является ли текущий блок первым
         */
        mutable bool m_isFirstBlock = true;

        /**
         * @brief Текст блока
         */
        mutable QString m_blockText;

        /**
         * @brief Текст редакторской заметки
         */
        mutable QString m_note;

        /**
         * @brief Редакторская заметка к текущему блоку
         * 		  tuple содержит комментарий, позиция и длина области редакторской заметки
         */
        mutable QVector<std::tuple<QString, unsigned, unsigned>> m_notes;

        /**
         * @brief Список форматов обрабатываемых блоков
         */
        mutable QVector<TextFormat> m_formats;

        /**
         * @brief Последний обрабатываемый формат
         */
        mutable TextFormat m_lastFormat;
    };
}

#endif // FOUNTAINIMPORTER_H
