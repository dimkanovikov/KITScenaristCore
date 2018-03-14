#include "FountainImporter.h"

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

#include <3rd_party/Helpers/TextEditHelper.h>

#include <QDomDocument>
#include <QFile>
#include <QStack>
#include <QXmlStreamWriter>

using namespace BusinessLogic;

namespace {
    /**
     * @brief Ключ для формирования xml из импортируемого документа
     */
    /** @{ */
    const QString NODE_SCENARIO = "scenario";
    const QString NODE_VALUE = "v";
    const QString NODE_FORMAT_GROUP = "formatting";
    const QString NODE_FORMAT = "format";

    const QString ATTRIBUTE_FORMAT_FROM = "from";
    const QString ATTRIBUTE_FORMAT_LENGTH = "length";
    const QString ATTRIBUTE_FORMAT_BOLD = "bold";
    const QString ATTRIBUTE_FORMAT_ITALIC = "italic";
    const QString ATTRIBUTE_FORMAT_UNDERLINE = "underline";

    const QString ATTRIBUTE_VERSION = "version";
    /** @} */

    /**
      * @brief С чего может начинаться название сцены
      */
    const QStringList sceneHeadings = {QApplication::translate("BusinessLayer::FountainImporter", "INT"),
                                       QApplication::translate("BusinessLayer::FountainImporter", "EXT"),
                                       QApplication::translate("BusinessLayer::FountainImporter", "EST"),
                                       QApplication::translate("BusinessLayer::FountainImporter", "INT./EXT"),
                                       QApplication::translate("BusinessLayer::FountainImporter", "INT/EXT"),
                                       QApplication::translate("BusinessLayer::FountainImporter", "I/E")};

    const QMap<QString, QString> TITLE_KEYS({{"Title", "name"},
                                             {"Author", "author"},
                                             {"Authors", "author"},
                                             {"Draft date", "year"},
                                             {"Copyright", "year"},
                                             {"Contact", "contacts"},
                                             {"Credit", "genre"},
                                             {"Source", "additional_info"}});

    const QString TRIPLE_WHITESPACE = "   ";
    const QString DOUBLE_WHITESPACE = "  ";

}

FountainImporter::FountainImporter() :
    AbstractImporter()
{
}

QString FountainImporter::importScript(const ImportParameters &_importParameters) const
{
    QString scenarioXml;

    //
    // Открываем файл
    //
    QFile fountainFile(_importParameters.filePath);
    if (fountainFile.open(QIODevice::ReadOnly)) {
        const QString& scriptText = fountainFile.readAll();
        scenarioXml = importScript(scriptText);
    }

    return scenarioXml;
}

QString FountainImporter::importScript(const QString& _scriptText) const {
    QString scenarioXml;
    //
    // Читаем plain text
    //
    // ... и пишем в сценарий
    //
    QXmlStreamWriter writer(&scenarioXml);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(true);
    writer.writeStartDocument();
    writer.writeStartElement(NODE_SCENARIO);
    writer.writeAttribute(ATTRIBUTE_VERSION, "1.0");

    //
    // Сформируем список строк, содержащий текст сценария
    //
    QVector<QString> paragraphs;
    bool isTitle = false;
    bool isFirstLine = true;
    for (QString& str : QString(_scriptText).split("\n")) {
        //
        // Если первая строка содержит ':', то в начале идет титульная страница,
        // которую мы обрабатываем не здесь
        //
        if (isFirstLine) {
            isFirstLine = false;
            if (str.contains(':')) {
                isTitle = true;
            }
        }

        if (isTitle) {
            //
            // Титульная страница заканчивается пустой строкой
            //
            if (str.simplified().isEmpty()) {
                isTitle = false;
            }
        } else {
            if (str.endsWith("\r")) {
                str.chop(1);
            }

            if (str == DOUBLE_WHITESPACE) {
                //
                // Если строка состоит из 2 пробелов, то это нужно сохранить
                // Используется для многострочных диалогов с пустыми строками
                //
                paragraphs.push_back(DOUBLE_WHITESPACE);
            } else {
                paragraphs.push_back(str.simplified());
            }
        }
    }

    //
    // Очищаем форматы перед импортом
    //
    m_formats.clear();
    m_lastFormat.clear();

    const int paragraphsCount = paragraphs.size();
    ScenarioBlockStyle::Type prevBlockType = ScenarioBlockStyle::Undefined;
    QStack<QString> dirs;
    ScenarioBlockStyle::Type blockType;
    for (int i = 0; i != paragraphsCount; ++i) {
        if (m_isNotation
                || m_isCommenting) {
            //
            // Если мы комментируем или делаем заметку, то продолжим это
            //
            processBlock(paragraphs[i], prevBlockType, writer);
            continue;
        }

        if (paragraphs[i].isEmpty()) {
            continue;
        }

        blockType = ScenarioBlockStyle::Action;
        QString paragraphText;

        switch(paragraphs[i][0].toLatin1()) {
            case '.':
            {
                blockType = ScenarioBlockStyle::SceneHeading;
                //
                // TODO: номера сцен игнорируем, поскольку в фонтане они являются строками
                //
                int sharpPos = paragraphs[i].size();
                if (paragraphs[i].endsWith("#")) {
                    sharpPos = paragraphs[i].lastIndexOf('#', paragraphs[i].size() - 2);
                }
                if (sharpPos == -1) {
                    sharpPos = paragraphs[i].size();
                }
                paragraphText = paragraphs[i].mid(1, sharpPos - 1);
                break;
            }

            case '!':
            {
                blockType = ScenarioBlockStyle::Action;
                paragraphText = paragraphs[i].mid(1);
                break;
            }

            case '@':
            {
                blockType = ScenarioBlockStyle::Character;
                paragraphText = paragraphs[i].mid(1);
                break;
            }

            case '>':
            {
                if (paragraphs[i].endsWith("<")) {
                    blockType = ScenarioBlockStyle::Action;
                    paragraphText = paragraphs[i].mid(1, paragraphs[i].size() - 2);
                } else {
                    blockType = ScenarioBlockStyle::Transition;
                    paragraphText = paragraphs[i].mid(1);
                }
                break;
            }

            case '=':
            {
                bool isPageBreak = false;
                if (paragraphs[i].startsWith("===")) {
                    isPageBreak = true;
                    for (int j = 3; j != paragraphs[i].size(); ++j) {
                        if (paragraphs[i][j] != '=') {
                            isPageBreak = false;
                            break;
                        }
                    }

                    //
                    // Если состоит из трех или более '=', то это PageBreak
                    // У нас такого сейчас нет
                    //
                    continue;
                }
                if (!isPageBreak) {
                    blockType = ScenarioBlockStyle::SceneDescription;
                    paragraphText = paragraphs[i].mid(1);
                }
                break;
            }

            case '~':
            {
                //
                // Лирика
                //
                blockType = ScenarioBlockStyle::Lyrics;
                paragraphText = paragraphs[i].mid(1);
                break;
            }

            case '#':
            {
                //
                // Директории
                //
                int sharpCount = 0;
                while (paragraphs[i][sharpCount] == '#') {
                    ++sharpCount;
                }

                if (sharpCount <= dirs.size()) {
                    //
                    // Закроем нужное число раз уже открытые
                    //
                    unsigned toClose = dirs.size() - sharpCount + 1;
                    for (unsigned i = 0; i != toClose; ++i) {
                        processBlock(QApplication::translate("FountainImporter", "END OF ") + dirs.top(),
                                     ScenarioBlockStyle::FolderFooter, writer);
                        dirs.pop();
                    }
                    prevBlockType = ScenarioBlockStyle::FolderFooter;
                }
                //
                // И откроем новую
                //
                QString text = paragraphs[i].mid(sharpCount);
                processBlock(text, ScenarioBlockStyle::FolderHeader, writer);
                dirs.push(text);
                prevBlockType = ScenarioBlockStyle::FolderHeader;

                //
                // Поскольку директории добавляются прямо здесь без обработки, то в конец цикла идти не надо
                //
                continue;
                break;
            }

            default:
            {
                bool startsWithHeading = false;
                for (const QString &sceneHeading : sceneHeadings) {
                    if (paragraphs[i].startsWith(sceneHeading)) {
                        startsWithHeading = true;
                        break;
                    }
                }

                if (startsWithHeading
                        && i + 1 < paragraphsCount
                        && paragraphs[i + 1].isEmpty()) {
                    //
                    // Если начинается с одного из времен действия, а после обязательно пустая строка
                    // Значит это заголовок сцены
                    //
                    blockType = ScenarioBlockStyle::SceneHeading;

                    //
                    // TODO: номера сцен игнорируем, поскольку в фонтане они являются строками
                    //
                    int sharpPos = paragraphs[i].size();
                    if (paragraphs[i].startsWith("#")) {
                        sharpPos = paragraphs[i].lastIndexOf('#', paragraphs[i].size() - 2);
                    }
                    if (sharpPos == -1) {
                        sharpPos = paragraphs[i].size();
                    }
                    paragraphText = paragraphs[i].left(sharpPos);
                } else if (paragraphs[i].startsWith("[[")
                           && paragraphs[i].endsWith("]]")) {
                    //
                    // Редакторская заметка
                    //
                    m_notes.append(std::make_tuple(paragraphs[i].mid(2, paragraphs[i].size() - 4), m_noteStartPos, m_noteLen));
                    m_noteStartPos += m_noteLen;
                    m_noteLen = 0;
                    continue;
                } else if (paragraphs[i].startsWith("/*")) {
                    //
                    // Начинается комментарий
                    paragraphText = paragraphs[i];
                } else if (paragraphs[i] == TextEditHelper::smartToUpper(paragraphs[i])
                           && i != 0
                           && paragraphs[i-1].isEmpty()
                           && i + 1 < paragraphsCount
                           && paragraphs[i+1].isEmpty()
                           && paragraphs[i].endsWith("TO:")) {
                    //
                    // Если состоит только из заглавных букв, предыдущая и следующая строки пустые
                    // и заканчивается "TO:", то это переход
                    //
                    blockType = ScenarioBlockStyle::Transition;
                    paragraphText = paragraphs[i].left(paragraphs[i].size()-4);
                } else if (paragraphs[i].startsWith("(")
                           && paragraphs[i].endsWith(")")
                           && (prevBlockType == ScenarioBlockStyle::Character
                               || prevBlockType == ScenarioBlockStyle::Dialogue)) {
                    //
                    // Если текущий блок обернут в (), то это ремарка
                    //
                    blockType = ScenarioBlockStyle::Parenthetical;
                    paragraphText = paragraphs[i];
                } else if (paragraphs[i] == TextEditHelper::smartToUpper(paragraphs[i])
                           && i != 0
                           && paragraphs[i-1].isEmpty()
                           && i + 1 < paragraphsCount
                           && !paragraphs[i+1].isEmpty()) {
                    //
                    // Если состоит из только из заглавных букв, впереди не пустая строка, а перед пустая
                    // Значит это имя персонажа (для реплики)
                    //
                    blockType = ScenarioBlockStyle::Character;
                    if (paragraphs[i].endsWith("^")) {
                        //
                        // Двойной диалог, который мы пока что не умеем обрабатывать
                        //
                        paragraphText = paragraphs[i].left(paragraphs[i].size() - 1);
                    } else {
                        paragraphText = paragraphs[i];
                    }
                } else if (prevBlockType == ScenarioBlockStyle::Character
                           || prevBlockType == ScenarioBlockStyle::Parenthetical
                           || (prevBlockType == ScenarioBlockStyle::Dialogue
                               && i > 0
                               && !paragraphs[i-1].isEmpty())) {
                    //
                    // Если предыдущий блок - имя персонажа или ремарка, то сейчас диалог
                    // Или предыдущая строка является диалогом
                    //
                    blockType = ScenarioBlockStyle::Dialogue;
                    paragraphText = paragraphs[i];
                } else {
                    //
                    // Во всех остальных случаях - Action
                    //
                    blockType = ScenarioBlockStyle::Action;
                    paragraphText = paragraphs[i];
                }
            }
        }
        //
        // Отправим блок на обработку
        //
        processBlock(paragraphText, blockType, writer);
        prevBlockType = blockType;
    }
    //
    // Добавим комментарии к последнему блоку
    //
    appendComments(writer);

    //
    // Закроем последний блок
    //
    writer.writeEndElement();

    //
    // Закроем директории нужное число раз
    //
    while (!dirs.empty()) {
        processBlock(QApplication::translate("FountainImporter", "END OF ") + dirs.top(),
                     ScenarioBlockStyle::FolderFooter, writer);
        dirs.pop();
    }

    //
    // Закроем документ
    //
    writer.writeEndElement();
    writer.writeEndDocument();

    return scenarioXml;
}


QVariantMap FountainImporter::importResearch(const ImportParameters &_importParameters) const
{
    //
    // Открываем файл
    //
    QVariantMap titlePage;
    QFile fountainFile(_importParameters.filePath);
    if (fountainFile.open(QIODevice::ReadOnly)) {
        //
        // Сохранить параметр, если он нам известен
        //
        auto saveParameter = [&titlePage] (const QString& _key, const QString& _value) {
            if (!_key.isEmpty()
                && !_value.isEmpty()
                && ::TITLE_KEYS.contains(_key)) {
                titlePage[::TITLE_KEYS[_key]] = _value;
            }
        };

        //
        // Титульная страница представлена в виде "key: value" и отделена от текста пустой строкой
        //
        QString lastKey;
        QString lastValue;
        forever {
            const QString textLine = fountainFile.readLine();
            QStringList textLineData = textLine.simplified().split(":", QString::SkipEmptyParts);
            //
            // Если в строке содержится ключ и значение
            //
            if (textLineData.size() > 1) {
                //
                // Если есть ещё не сохранённый параметр, сохраним
                //
                saveParameter(lastKey, lastValue);
                //
                // ... и очистим
                //
                lastKey.clear();
                lastValue.clear();

                //
                // Сохраним текущий параметр
                //
                const QString key = textLineData.takeFirst();
                const QString value = textLineData.join(":").simplified();
                saveParameter(key, value);
            }
            //
            // Иначе в строке содержится либо ключ, либо значение, либо это конец титульной страницы
            //
            else {
                //
                // Ключ
                //
                if (textLine.contains(":")) {
                    lastKey = textLineData.first();
                    lastValue.clear();
                }
                //
                // Значение
                // Каждая строка должна начинаться либо с табуляции, либо с 3 пробелов минимум
                //
                else if (!lastKey.isEmpty()
                         && !textLineData.isEmpty()
                         && !textLineData.first().isEmpty()
                         && (textLine.startsWith(TRIPLE_WHITESPACE)
                             || textLine.startsWith('\t'))) {
                    if (!lastValue.isEmpty()) {
                        lastValue.append("\n");
                    }
                    lastValue.append(textLineData.first());
                }
                //
                // Конец титульной страницы
                //
                else {
                    //
                    // Сохраняем последний несохранённый параметр
                    //
                    saveParameter(lastKey, lastValue);
                    //
                    // ... и прерываем выполнение
                    //
                    break;
                }
            }
        }
    }

    QVariantMap result;
    result["script"] = titlePage;
    return result;
}

void FountainImporter::processBlock(const QString& _paragraphText, ScenarioBlockStyle::Type _type,
    QXmlStreamWriter& _writer) const
{
    if (!m_isNotation
        && !m_isCommenting) {
        //
        // Начинается новая сущность
        //
        m_blockText.reserve(_paragraphText.size());

        //
        // Добавим комментарии к предыдущему блоку
        //
        appendComments(_writer);

        m_noteLen = 0;
        m_noteStartPos = 0;
    }

    if (!m_isCommenting) {
        m_formats.clear();
    }

    char prevSymbol = '\0';
    int asteriskLen = 0;
    for (int i = 0; i != _paragraphText.size(); ++i) {
        //
        // Если предыдущий символ - \, то просто добавим текущий
        //
        if (prevSymbol == '\\') {
            if (m_isNotation) {
                m_note.append(_paragraphText[i]);
            } else {
                m_blockText.append(_paragraphText[i]);
            }
            continue;
        }

        char curSymbol = _paragraphText[i].toLatin1();
        switch (curSymbol) {
            case '\\':
            {
                if (m_isNotation) {
                    m_note.append(_paragraphText[i]);
                } else {
                    m_blockText.append(_paragraphText[i]);
                }
                break;
            }

            case '/':
            {
                if (prevSymbol == '*'
                    && m_isCommenting) {
                    //
                    // Заканчивается комментирование
                    //
                    --asteriskLen;
                    m_isCommenting = false;
                    m_noteStartPos += m_noteLen;
                    m_noteLen = m_blockText.size();

                    //
                    // Закроем предыдущий блок, добавим текущий
                    //
                    _writer.writeEndElement();
                    appendBlock(m_blockText.left(m_blockText.size()),
                                ScenarioBlockStyle::NoprintableText, _writer);
                    m_blockText.clear();
                } else {
                    if (m_isNotation) {
                        m_note.append('/');
                    } else {
                        m_blockText.append('/');
                    }
                }
                break;
            }

            case '*':
            {
                if (prevSymbol == '/'
                    && !m_isCommenting
                    && !m_isNotation) {
                    //
                    // Начинается комментирование
                    //
                    m_isCommenting = true;
                    m_noteStartPos += m_noteLen;
                    m_noteLen = m_blockText.size() - 1;

                    //
                    // Закроем предыдущий блок и, если комментирование начинается в середние текущего блока
                    // то добавим этот текущий блок
                    //
                    _writer.writeEndElement();
                    if (m_blockText.size() != 1) {
                        appendBlock(m_blockText.left(m_blockText.size() - 1), _type, _writer);
                        appendComments(_writer);
                        m_notes.clear();
                    }
                    m_blockText.clear();
                } else {
                    if (m_isNotation) {
                        m_note.append('*');
                    } else {
                        ++asteriskLen;
                    }
                }
                break;
            }

            case '[':
            {
                if (prevSymbol == '['
                        && !m_isCommenting
                        && !m_isNotation) {
                    //
                    // Начинается редакторская заметка
                    //
                    m_isNotation = true;
                    m_noteLen = m_blockText.size() - 1 - m_noteStartPos;
                    m_blockText = m_blockText.left(m_blockText.size() - 1);
                } else {
                    if (m_isNotation) {
                        m_note.append('[');
                    } else {
                        m_blockText.append('[');
                    }
                }
                break;
            }

            case ']':
            {
                if (prevSymbol == ']'
                        && m_isNotation) {
                    //
                    // Закончилась редакторская заметка. Добавим ее в список редакторских заметок к текущему блоку
                    //
                    m_isNotation = false;
                    m_notes.append(std::make_tuple(m_note.left(m_note.size() - 1), m_noteStartPos, m_noteLen));
                    m_noteStartPos += m_noteLen;
                    m_noteLen = 0;
                    m_note.clear();
                } else {
                    if (m_isNotation) {
                        m_note.append(']');
                    } else {
                        m_blockText.append(']');
                    }
                }
                break;
            }

            case '_':
            {
                //
                // Подчеркивания обрабатываются в другом месте, поэтому тут игнорируем его обработку
                //
                break;
            }

            default:
            {
                //
                // Самый обычный символ
                //
                if (m_isNotation) {
                    m_note.append(_paragraphText[i]);
                } else {
                    m_blockText.append(_paragraphText[i]);
                }
                break;
            }
        }

        //
        // Underline
        //
        if (prevSymbol == '_') {
            processFormat(false, false, true, curSymbol == '*');
        }

        if (curSymbol != '*') {
            switch (asteriskLen) {
                //
                // Italics
                //
                case 1:
                {
                    processFormat(true, false, false, curSymbol == '_');
                    break;
                }

                //
                // Bold
                //
                case 2:
                {
                    processFormat(false, true, false, curSymbol == '_');
                    break;
                }

                //
                // Bold & Italics
                //
                case 3:
                {
                    processFormat(true, true, false, curSymbol == '_');
                    break;
                }

                default: break;
            }
            asteriskLen = 0;
        }

        prevSymbol = curSymbol;
    }

    //
    // Underline
    //
    if (prevSymbol == '_') {
        processFormat(false, false, true, true);
    }

    switch(asteriskLen) {
        //
        // Italics
        //
        case 1:
        {
            processFormat(true, false, false, true);
            break;
        }

        //
        // Bold
        //
        case 2:
        {
            processFormat(false, true, false, true);
            break;
        }

        //
        // Bold & Italics
        //
        case 3:
        {
            processFormat(true, true, false, true);
            break;
        }

        default: break;
    }
    asteriskLen = 0;


    if (!m_isNotation
        && !m_isCommenting) {
        //
        // Если блок действительно закончился
        //
        m_noteLen += m_blockText.size() - m_noteStartPos;

        //
        // Закроем предыдущий блок
        //
        if (!m_isFirstBlock) {
            _writer.writeEndElement();
        }

        //
        // Добавим текущий блок
        //
        if (!m_blockText.isEmpty() || _type == ScenarioBlockStyle::FolderFooter) {
            appendBlock(m_blockText, _type, _writer);
        }
        m_blockText.clear();
    }

    //
    // Первый блок в тексте может встретиться лишь однажды
    //
    if (!m_isFirstBlock) {
        m_isFirstBlock = false;
    }
}

void FountainImporter::appendBlock(const QString& _paragraphText, ScenarioBlockStyle::Type _type,
    QXmlStreamWriter& _writer) const
{
    int leadSpaceCount = 0;
    QString paragraphText = _paragraphText;
    while (!paragraphText.isEmpty()
           && paragraphText.startsWith(" ")) {
        ++leadSpaceCount;
        paragraphText = paragraphText.mid(1);
    }

    const QString& blockTypeName = ScenarioBlockStyle::typeName(_type);
    _writer.writeStartElement(blockTypeName);
    _writer.writeStartElement(NODE_VALUE);
    _writer.writeCDATA(paragraphText);
    _writer.writeEndElement();

    //
    // Если есть форматирование, которое распространяется на несколько блоков
    //
    if (m_lastFormat.isValid()) {
        //
        // Добавим его в список форматов текущего блока
        //
        m_lastFormat.length = _paragraphText.trimmed().length() - m_lastFormat.start;
        m_formats.append(m_lastFormat);
        //
        // Для следующего блока он будет начинаться с первого символа
        //
        m_lastFormat.start = m_lastFormat.length = 0;
    }

    //
    // Пишем форматирование, если оно есть
    //
    if (!m_formats.isEmpty()) {
        _writer.writeStartElement(NODE_FORMAT_GROUP);
        for (const TextFormat& format : m_formats) {
            _writer.writeStartElement(NODE_FORMAT);
            //
            // Данные пользовательского форматирования
            //
            _writer.writeAttribute(ATTRIBUTE_FORMAT_FROM, QString::number(format.start - leadSpaceCount));
            _writer.writeAttribute(ATTRIBUTE_FORMAT_LENGTH, QString::number(format.length));
            _writer.writeAttribute(ATTRIBUTE_FORMAT_BOLD, format.bold ? "true" : "false");
            _writer.writeAttribute(ATTRIBUTE_FORMAT_ITALIC, format.italic? "true" : "false");
            _writer.writeAttribute(ATTRIBUTE_FORMAT_UNDERLINE, format.underline ? "true" : "false");
            //
            _writer.writeEndElement();
        }
        _writer.writeEndElement();
        m_formats.clear();
    }

    //
    // Не закрываем блок, чтобы можно было добавить редакторских заметок
    //
}

void FountainImporter::appendComments(QXmlStreamWriter& _writer) const
{
    if (m_notes.isEmpty()) {
        return;
    }

    _writer.writeStartElement("reviews");

    for (int i = 0; i != m_notes.size(); ++i) {
        if (std::get<2>(m_notes[i]) != 0) {
            if (i != 0) {
                _writer.writeEndElement();
            }
            _writer.writeStartElement("review");
            _writer.writeAttribute("from", QString::number(std::get<1>(m_notes[i])));
            _writer.writeAttribute("length", QString::number(std::get<2>(m_notes[i])));
            _writer.writeAttribute("color", "#000000");
            _writer.writeAttribute("bgcolor", "#ffff00");
            _writer.writeAttribute("is_highlight", "true");
        }
        _writer.writeEmptyElement("review_comment");
        _writer.writeAttribute("comment", std::get<0>(m_notes[i]));
    }

    _writer.writeEndElement();
    _writer.writeEndElement();

    m_notes.clear();
}

QString FountainImporter::simplify(const QString& _value) const
{
    QString res;
    for (int i = 0; i != _value.size(); ++i) {
        if (_value[i] == '*'
            || _value[i] == '_'
            || _value[i] == '\\') {
            if (i == 0
                || (i > 0
                    && _value[i-1] != '\\')) {
                continue;
            } else {
                res += _value[i];
            }
        }
        else {
            res += _value[i];
        }
    }
    return res;
}

void FountainImporter::processFormat(bool _italics, bool _bold, bool _underline,
                                     bool _forCurrentCharacter) const
{
    //
    // Новый формат, который еще не начат
    //
    if (!m_lastFormat.isValid()) {
        m_lastFormat.bold = _bold;
        m_lastFormat.italic = _italics;
        m_lastFormat.underline = _underline;
        m_lastFormat.start = m_blockText.size();
        if (!_forCurrentCharacter) {
            --m_lastFormat.start;
        }
    }
    //
    // Формат уже начат
    //
    else {
        //
        // Добавим его в список форматов
        //
        m_lastFormat.length = m_blockText.size() - m_lastFormat.start;
        if (!_forCurrentCharacter) {
            --m_lastFormat.length;
        }
        if (m_lastFormat.length != 0) {
            m_formats.push_back(m_lastFormat);
        }

        //
        // Если необходимо, созданим новый, частично унаследованный от текущего
        //
        if (m_lastFormat.bold != _bold
            || m_lastFormat.italic != _italics
            || m_lastFormat.underline != _underline) {
            m_lastFormat.italic = m_lastFormat.italic ^ _italics;
            m_lastFormat.bold = m_lastFormat.bold ^ _bold;
            m_lastFormat.underline = m_lastFormat.underline ^ _underline;
            m_lastFormat.start = m_lastFormat.start + m_lastFormat.length;
        }
        //
        // Либо просто закроем
        //
        else {
            m_lastFormat.clear();
        }
    }
}

