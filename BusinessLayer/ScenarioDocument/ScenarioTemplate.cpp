#include "ScenarioTemplate.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <3rd_party/Helpers/TextEditHelper.h>
#include <3rd_party/Widgets/PagesTextEdit/PageMetrics.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontInfo>
#include <QFontMetrics>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QStringListModel>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QXmlStreamReader>

using BusinessLogic::ScenarioBlockStyle;
using BusinessLogic::ScenarioTemplate;
using BusinessLogic::ScenarioTemplateFacade;

using BusinessLogic::ScenarioBlockStyle;


// ********
// ScenarioBlockStyle

namespace {
    /**
     * @brief Получить карту типов и их текстовых отображений
     */
    static QMap<ScenarioBlockStyle::Type, QString> typeNames() {
        static QMap<ScenarioBlockStyle::Type, QString> s_typeNames;
        if (s_typeNames.isEmpty()) {
            s_typeNames.insert(ScenarioBlockStyle::Undefined, "undefined");
            s_typeNames.insert(ScenarioBlockStyle::SceneHeading, "scene_heading");
            s_typeNames.insert(ScenarioBlockStyle::SceneCharacters, "scene_characters");
            s_typeNames.insert(ScenarioBlockStyle::Action, "action");
            s_typeNames.insert(ScenarioBlockStyle::Character, "character");
            s_typeNames.insert(ScenarioBlockStyle::Parenthetical, "parenthetical");
            s_typeNames.insert(ScenarioBlockStyle::Dialogue, "dialog");
            s_typeNames.insert(ScenarioBlockStyle::Transition, "transition");
            s_typeNames.insert(ScenarioBlockStyle::Note, "note");
            s_typeNames.insert(ScenarioBlockStyle::TitleHeader, "title_header");
            s_typeNames.insert(ScenarioBlockStyle::Title, "title");
            s_typeNames.insert(ScenarioBlockStyle::NoprintableText, "noprintable_text");
            s_typeNames.insert(ScenarioBlockStyle::FolderHeader, "folder_header");
            s_typeNames.insert(ScenarioBlockStyle::FolderFooter, "folder_footer");
            s_typeNames.insert(ScenarioBlockStyle::SceneDescription, "scene_description");
            s_typeNames.insert(ScenarioBlockStyle::Lyrics, "lyrics");
        }
        return s_typeNames;
    }

    /**
     * @brief Получить карту типов и их текстовых отображений в красивом виде
     */
    static QMap<ScenarioBlockStyle::Type, QString> beautifyTypeNames() {
        static QMap<ScenarioBlockStyle::Type, QString> s_beautifyTypeNames;
        if (s_beautifyTypeNames.isEmpty()) {
            s_beautifyTypeNames.insert(ScenarioBlockStyle::Undefined, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Undefined"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::SceneHeading, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Scene Heading"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::SceneCharacters, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Scene Characters"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::Action, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Action"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::Character, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Character"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::Parenthetical, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Parenthetical"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::Dialogue, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Dialog"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::Transition, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Transition"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::Note, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Shot"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::TitleHeader, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Title Header"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::Title, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Title"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::NoprintableText, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Noprintable Text"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::FolderHeader, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Folder"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::FolderFooter, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Folder Footer"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::SceneDescription, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Scene Description"));
            s_beautifyTypeNames.insert(ScenarioBlockStyle::Lyrics, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Lyrics"));
        }
        return s_beautifyTypeNames;
    }

    /**
     * @brief Получить карту типов и их кратких текстовых отображений
     */
    static QMap<ScenarioBlockStyle::Type, QString> shortTypeNames() {
        static QMap<ScenarioBlockStyle::Type, QString> s_shortTypeNames;
        if (s_shortTypeNames.isEmpty()) {
            s_shortTypeNames.insert(ScenarioBlockStyle::Undefined, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Undefined"));
            //: Reduction of Scene Heading
            s_shortTypeNames.insert(ScenarioBlockStyle::SceneHeading, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "SH"));
            //: Reduction of Scene Characters
            s_shortTypeNames.insert(ScenarioBlockStyle::SceneCharacters, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "SC"));
            //: Reduction of Action
            s_shortTypeNames.insert(ScenarioBlockStyle::Action, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "A"));
            //: Reduction of Character
            s_shortTypeNames.insert(ScenarioBlockStyle::Character, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "C"));
            //: Reduction of Parenthetical
            s_shortTypeNames.insert(ScenarioBlockStyle::Parenthetical, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "P"));
            //: Reduction of Dialog
            s_shortTypeNames.insert(ScenarioBlockStyle::Dialogue, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "D"));
            //: Reduction of Transition
            s_shortTypeNames.insert(ScenarioBlockStyle::Transition, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Tr"));
            //: Reduction of Shot
            s_shortTypeNames.insert(ScenarioBlockStyle::Note, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "S"));
            //: Reduction of Title Header
            s_shortTypeNames.insert(ScenarioBlockStyle::TitleHeader, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "TH"));
            //: Reduction of Title
            s_shortTypeNames.insert(ScenarioBlockStyle::Title, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "Ti"));
            //: Reduction of Noprintable Text
            s_shortTypeNames.insert(ScenarioBlockStyle::NoprintableText, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "NT"));
            //: Reduction of Folder
            s_shortTypeNames.insert(ScenarioBlockStyle::FolderHeader, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "F"));
            //: Reduction of Folder Footer
            s_shortTypeNames.insert(ScenarioBlockStyle::FolderFooter, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "FF"));
            //: Reduction of Scene Description
            s_shortTypeNames.insert(ScenarioBlockStyle::SceneDescription, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "SD"));
            //: Reduction of Lyrics
            s_shortTypeNames.insert(ScenarioBlockStyle::Lyrics, QApplication::translate("BusinessLogic::ScenarioBlockStyle", "L"));
        }
        return s_shortTypeNames;
    }

    /**
     * @brief Расширение файла стиля сценария
     */
    const QString SCENARIO_TEMPLATE_FILE_EXTENSION = "kitss";

    /**
     * @brief Получить выравнивание из строки
     */
    static Qt::Alignment alignmentFromString(const QString& _alignment) {
        Qt::Alignment result;
        //
        // Если нужно преобразовать несколько условий
        //
        if (_alignment.contains(",")) {
            foreach (const QString& align, _alignment.split(",")) {
                result |= alignmentFromString(align);
            }
        }
        //
        // Если нужно преобразовать одно выравнивание
        //
        else {
            if (_alignment == "top") result = Qt::AlignTop;
            else if (_alignment == "bottom") result = Qt::AlignBottom;
            else if (_alignment == "left") result = Qt::AlignLeft;
            else if (_alignment == "center") result = Qt::AlignCenter;
            else if (_alignment == "right") result = Qt::AlignRight;
            else if (_alignment == "justify") result = Qt::AlignJustify;
        }
        return result;
    }

    /**
     * @brief Получить межстрочный интервал из строки
     */
    static ScenarioBlockStyle::LineSpacing lineSpacingFromString(const QString& _lineSpacing) {
        ScenarioBlockStyle::LineSpacing result = ScenarioBlockStyle::SingleLineSpacing;

        if (_lineSpacing == "single") result = ScenarioBlockStyle::SingleLineSpacing;
        else if (_lineSpacing == "oneandhalf") result = ScenarioBlockStyle::OneAndHalfLineSpacing;
        else if (_lineSpacing == "double") result = ScenarioBlockStyle::DoubleLineSpacing;
        else if (_lineSpacing == "fixed") result = ScenarioBlockStyle::FixedLineSpacing;

        return result;
    }
}

QString ScenarioBlockStyle::typeName(ScenarioBlockStyle::Type _type, bool _beautify)
{
    return _beautify ? ::beautifyTypeNames().value(_type) : ::typeNames().value(_type);
}

ScenarioBlockStyle::Type ScenarioBlockStyle::typeForName(const QString& _typeName, bool _beautify)
{
    return _beautify ? ::beautifyTypeNames().key(_typeName) : ::typeNames().key(_typeName);
}

QString ScenarioBlockStyle::shortTypeName(ScenarioBlockStyle::Type _type)
{
    return ::shortTypeNames().value(_type);
}

ScenarioBlockStyle::Type ScenarioBlockStyle::forBlock(const QTextBlock& _block)
{
    ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::Undefined;
    if (_block.blockFormat().hasProperty(ScenarioBlockStyle::PropertyType)) {
        blockType = (ScenarioBlockStyle::Type)_block.blockFormat().intProperty(ScenarioBlockStyle::PropertyType);
    }
    return blockType;
}

void ScenarioBlockStyle::setType(ScenarioBlockStyle::Type _type)
{
    if (m_type != _type) {
        m_type = _type;
        m_blockFormat.setProperty(ScenarioBlockStyle::PropertyType, _type);
    }
}

void ScenarioBlockStyle::setIsActive(bool _isActive)
{
    if (m_isActive != _isActive) {
        m_isActive = _isActive;
    }
}

void ScenarioBlockStyle::setFont(const QFont& _font)
{
    if (m_font != _font) {
        m_font = _font;

        m_charFormat.setFont(m_font);
        updateLineHeight();
    }
}

void ScenarioBlockStyle::setAlign(Qt::Alignment _align)
{
    if (m_align != _align) {
        m_align = _align;

        m_blockFormat.setAlignment(m_align);
        m_blockFormat.setTopMargin(QFontMetricsF(m_font).lineSpacing() * m_topSpace);
    }
}

void ScenarioBlockStyle::setTopSpace(int _topSpace)
{
    if (m_topSpace != _topSpace) {
        m_topSpace = _topSpace;

        updateTopMargin();
    }
}

void ScenarioBlockStyle::setBottomSpace(int _bottomSpace)
{
    if (m_bottomSpace != _bottomSpace) {
        m_bottomSpace = _bottomSpace;

        updateBottomMargin();
    }
}

void ScenarioBlockStyle::setLeftMargin(qreal _leftMargin)
{
    if (m_leftMargin != _leftMargin) {
        m_leftMargin = _leftMargin;

        m_blockFormat.setLeftMargin(PageMetrics::mmToPx(m_leftMargin));
    }
}

void ScenarioBlockStyle::setTopMargin(qreal _topMargin)
{
    if (m_topMargin != _topMargin) {
        m_topMargin = _topMargin;

        updateTopMargin();
    }
}

void ScenarioBlockStyle::setRightMargin(qreal _rightMargin)
{
    if (m_rightMargin != _rightMargin) {
        m_rightMargin = _rightMargin;

        m_blockFormat.setRightMargin(PageMetrics::mmToPx(m_rightMargin));
    }
}

void ScenarioBlockStyle::setBottomMargin(qreal _bottomMargin)
{
    if (m_bottomMargin != _bottomMargin) {
        m_bottomMargin = _bottomMargin;

        updateBottomMargin();
    }
}

void ScenarioBlockStyle::setLineSpacing(ScenarioBlockStyle::LineSpacing _lineSpacing)
{
    if (m_lineSpacing != _lineSpacing) {
        m_lineSpacing = _lineSpacing;

        updateLineHeight();
    }
}

void ScenarioBlockStyle::setLineSpacingValue(qreal _value)
{
    if (m_lineSpacingValue != _value) {
        m_lineSpacingValue = _value;

        updateLineHeight();
    }
}

QTextBlockFormat ScenarioBlockStyle::blockFormat() const
{
    return m_blockFormat;
}

void ScenarioBlockStyle::setBackgroundColor(const QColor& _color)
{
    m_blockFormat.setBackground(_color);
}

void ScenarioBlockStyle::setTextColor(const QColor& _color)
{
    m_charFormat.setForeground(_color);
}

bool ScenarioBlockStyle::isFirstUppercase() const
{
    return m_charFormat.boolProperty(ScenarioBlockStyle::PropertyIsFirstUppercase);
}

bool ScenarioBlockStyle::isCanModify() const
{
    return m_charFormat.boolProperty(ScenarioBlockStyle::PropertyIsCanModify);
}

void ScenarioBlockStyle::setCanModify(bool _can)
{
    if (isCanModify() != _can) {
        m_charFormat.setProperty(ScenarioBlockStyle::PropertyIsCanModify, _can);
    }
}

bool ScenarioBlockStyle::hasDecoration() const
{
    return !prefix().isEmpty() || !postfix().isEmpty();
}

QString ScenarioBlockStyle::prefix() const
{
    return m_charFormat.stringProperty(ScenarioBlockStyle::PropertyPrefix);
}

void ScenarioBlockStyle::setPrefix(const QString& _prefix)
{
    if (prefix() != _prefix) {
        m_charFormat.setProperty(ScenarioBlockStyle::PropertyPrefix, _prefix);
    }
}

QString ScenarioBlockStyle::postfix() const
{
    return m_charFormat.stringProperty(ScenarioBlockStyle::PropertyPostfix);
}

void ScenarioBlockStyle::setPostfix(const QString& _postfix)
{
    if (postfix() != _postfix) {
        m_charFormat.setProperty(ScenarioBlockStyle::PropertyPostfix, _postfix);
    }
}

bool ScenarioBlockStyle::hasHeader() const
{
    return !header().isEmpty();
}

ScenarioBlockStyle::Type ScenarioBlockStyle::headerType() const
{
    return (ScenarioBlockStyle::Type)m_blockFormat.intProperty(ScenarioBlockStyle::PropertyHeaderType);
}

QString ScenarioBlockStyle::header() const
{
    return m_blockFormat.stringProperty(ScenarioBlockStyle::PropertyHeader);
}

bool ScenarioBlockStyle::isHeader() const
{
    return m_type == ScenarioBlockStyle::TitleHeader;
}

bool ScenarioBlockStyle::isEmbeddable() const
{
    return  m_type == FolderHeader
            || m_type == FolderFooter;
}

bool ScenarioBlockStyle::isEmbeddableHeader() const
{
    return  m_type == FolderHeader;
}

ScenarioBlockStyle::Type ScenarioBlockStyle::embeddableFooter() const
{
    ScenarioBlockStyle::Type footer = Undefined;

    if (m_type == FolderHeader) {
        footer = FolderFooter;
    }

    return footer;
}

ScenarioBlockStyle::ScenarioBlockStyle(const QXmlStreamAttributes& _blockAttributes)
{
    //
    // Считываем параметры
    //
    // ... тип блока и его активность в стиле
    // NOTE: обрабатываем так же и старые стили
    //
    QString typeName = _blockAttributes.value("id").toString();
    if (typeName == "time_and_place") {
        typeName = "scene_heading";
    }
    m_type = typeForName(typeName);
    m_isActive = _blockAttributes.value("active").toString() == "true";
    //
    // ... настройки шрифта
    //
    m_font.setFamily(_blockAttributes.value("font_family").toString());
    m_font.setPointSizeF(_blockAttributes.value("font_size").toDouble());
    //
    // ... начертание
    //
    m_font.setBold(_blockAttributes.value("bold").toString() == "true");
    m_font.setItalic(_blockAttributes.value("italic").toString() == "true");
    m_font.setUnderline(_blockAttributes.value("underline").toString() == "true");
    m_font.setCapitalization(_blockAttributes.value("uppercase").toString() == "true"
                             ? QFont::AllUppercase : QFont::MixedCase);

    //
    // ... расположение блока
    //
    m_align = ::alignmentFromString(_blockAttributes.value("alignment").toString());
    m_topSpace = _blockAttributes.value("top_space").toInt();
    m_bottomSpace = _blockAttributes.value("bottom_space").toInt();
    m_leftMargin = _blockAttributes.value("left_margin").toDouble();
    m_topMargin = _blockAttributes.value("top_margin").toDouble();
    m_rightMargin = _blockAttributes.value("right_margin").toDouble();
    m_bottomMargin = _blockAttributes.value("bottom_margin").toDouble();
    m_lineSpacing = ::lineSpacingFromString(_blockAttributes.value("line_spacing").toString());
    m_lineSpacingValue = _blockAttributes.value("line_spacing_value").toDouble();

    //
    // Настроим форматы
    //
    // ... блока
    //
    m_blockFormat.setAlignment(m_align);
    m_blockFormat.setLeftMargin(PageMetrics::mmToPx(m_leftMargin));
    m_blockFormat.setRightMargin(PageMetrics::mmToPx(m_rightMargin));
    updateLineHeight();
    //
    // ... текста
    //
    m_charFormat.setFont(m_font);

    //
    // Запомним в стиле его настройки
    //
    m_blockFormat.setProperty(ScenarioBlockStyle::PropertyType, m_type);
    m_blockFormat.setProperty(ScenarioBlockStyle::PropertyHeaderType,
                                     ScenarioBlockStyle::Undefined);
    m_charFormat.setProperty(ScenarioBlockStyle::PropertyIsFirstUppercase, true);
    m_charFormat.setProperty(ScenarioBlockStyle::PropertyIsCanModify, true);

    //
    // Настроим остальные характеристики
    //
    switch (m_type) {
        case Parenthetical: {
            m_charFormat.setProperty(ScenarioBlockStyle::PropertyIsFirstUppercase, false);
            //
            // Стандартное обрамление
            //
            m_charFormat.setProperty(ScenarioBlockStyle::PropertyPrefix, "(");
            m_charFormat.setProperty(ScenarioBlockStyle::PropertyPostfix, ")");
            break;
        }

        case TitleHeader: {
            m_charFormat.setProperty(ScenarioBlockStyle::PropertyIsCanModify, false);
            break;
        }

        case Title: {
            m_blockFormat.setProperty(ScenarioBlockStyle::PropertyHeaderType,
                                             ScenarioBlockStyle::TitleHeader);
            m_blockFormat.setProperty(ScenarioBlockStyle::PropertyHeader,
                                             QObject::tr("Title:", "ScenarioBlockStyle"));
            break;
        }

        case FolderFooter: {
            m_charFormat.setProperty(ScenarioBlockStyle::PropertyIsCanModify, false);
            break;
        }

        default: {
            break;
        }
    }
    //
    // ... обрамление блока
    //
    const QString prefix = _blockAttributes.value("prefix").toString();
    if (!prefix.isEmpty()) {
        m_charFormat.setProperty(ScenarioBlockStyle::PropertyPrefix, prefix);
    }
    const QString postfix = _blockAttributes.value("postfix").toString();
    if (!postfix.isEmpty()) {
        m_charFormat.setProperty(ScenarioBlockStyle::PropertyPostfix, postfix);
    }
}

void ScenarioBlockStyle::updateLineHeight()
{
    qreal lineHeight = TextEditHelper::fontLineHeight(m_font);;
    switch (m_lineSpacing) {
        case FixedLineSpacing: {
            lineHeight = PageMetrics::mmToPx(m_lineSpacingValue);
            break;
        }

        case DoubleLineSpacing: {
            lineHeight *= 2;
            break;
        }

        case OneAndHalfLineSpacing: {
            lineHeight *= 1.5;
            break;
        }

        case SingleLineSpacing:
        default: {
            break;
        }
    }
    m_blockFormat.setLineHeight(lineHeight, QTextBlockFormat::FixedHeight);

    updateTopMargin();
    updateBottomMargin();
}

void ScenarioBlockStyle::updateTopMargin()
{
    m_blockFormat.setTopMargin(
        m_blockFormat.lineHeight() * m_topSpace + PageMetrics::mmToPx(m_topMargin)
        );
}

void ScenarioBlockStyle::updateBottomMargin()
{
    m_blockFormat.setBottomMargin(
        m_blockFormat.lineHeight() * m_bottomSpace + PageMetrics::mmToPx(m_bottomMargin)
        );
}

// ********
// ScenarioTemplate

namespace {
    /**
     * @brief Получить отступы из строки
     */
    static QMarginsF marginsFromString(const QString& _margins) {
        QStringList margins = _margins.split(",");
        return QMarginsF(margins.value(0, 0).simplified().toDouble(),
                         margins.value(1, 0).simplified().toDouble(),
                         margins.value(2, 0).simplified().toDouble(),
                         margins.value(3, 0).simplified().toDouble());
    }

    /**
     * @brief Получить строку из отступов
     */
    static QString stringFromMargins(const QMarginsF& _margins) {
        return QString("%1,%2,%3,%4")
                .arg(_margins.left())
                .arg(_margins.top())
                .arg(_margins.right())
                .arg(_margins.bottom());
    }

    /**
     * @brief Преобразование разных типов в строку для записи в xml
     */
    /** @{ */
    static QString toString(bool _value)  { return _value ? "true" : "false"; }
    static QString toString(int _value)   { return QString::number(_value); }
    static QString toString(qreal _value) { return QString::number(_value); }
    static QString toString(ScenarioBlockStyle::Type _value) {
        return ScenarioBlockStyle::typeName(_value);
    }
    static QString toString(Qt::Alignment _value) {
        QString result;

        if (_value.testFlag(Qt::AlignTop)) result.append("top");
        if (_value.testFlag(Qt::AlignBottom)) result.append("bottom");

        if (!result.isEmpty()) result.append(",");

        if (_value.testFlag(Qt::AlignLeft)) result.append("left");
        if (_value.testFlag(Qt::AlignCenter)) result.append("center");
        if (_value.testFlag(Qt::AlignRight)) result.append("right");
        if (_value.testFlag(Qt::AlignJustify)) result.append("justify");

        return result;
    }
    static QString toString(ScenarioBlockStyle::LineSpacing _value) {
        QString result;

        switch (_value) {
            default:
            case ScenarioBlockStyle::SingleLineSpacing: result = "single"; break;
            case ScenarioBlockStyle::OneAndHalfLineSpacing: result = "oneandhalf"; break;
            case ScenarioBlockStyle::DoubleLineSpacing: result = "double"; break;
            case ScenarioBlockStyle::FixedLineSpacing: result = "fixed"; break;
        }

        return result;
    }
    /** @} */
}

QString ScenarioTemplate::finalDraftA4TemplateName()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Final Draft screenplay (A4)");
}

QString ScenarioTemplate::finalDraftLetterTemplateName()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Final Draft screenplay (Letter)");
}

QString ScenarioTemplate::russianTemplateName()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Russian screenplay");
}

QString ScenarioTemplate::russianTemplateWithCourierPrimeTemplateName()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Russian screenplay with Courier Prime");
}

QString ScenarioTemplate::chineseTemplateName()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Chinese screenplay");
}

QString ScenarioTemplate::hebrewTemplateName()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Hebrew screenplay");
}

QString ScenarioTemplate::arabicTemplateName()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Arabic screenplay");
}

QString ScenarioTemplate::finalDraftA4TemplateDescription()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "International screenplay standard template for A4 page size.");
}

QString ScenarioTemplate::finalDraftLetterTemplateDescription()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "International screenplay standard template for Letter page size.");
}

QString ScenarioTemplate::russianTemplateDescription()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Russian and CIS screenplay standard template.");
}

QString ScenarioTemplate::russianTemplateWithCourierPrimeTemplateDescription()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Russian and CIS screenplay standard template with Courier Prime font instead of Courier New.");
}

QString ScenarioTemplate::chineseTemplateDescription()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Chinese screenplay standard template.");
}

QString ScenarioTemplate::hebrewTemplateDescription()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Hebrew screenplay standard template.");
}

QString ScenarioTemplate::arabicTemplateDescription()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Arabic screenplay standard template.");
}

QString ScenarioTemplate::oldFinalDraftA4TemplateName()
{
    return "Final Draft A4";
}

QString ScenarioTemplate::oldFinalDraftLetterTemplateName()
{
    return "Final Draft Letter";
}

QString ScenarioTemplate::oldRussianTemplateName()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Default");
}

QString ScenarioTemplate::oldRussianWithCourierPrimeTemplateName()
{
    return QApplication::translate("BusinessLogic::ScenarioTemplate", "Default with Courier Prime");
}

QString ScenarioTemplate::oldChineseTemplateName()
{
    return "Chinese script template";
}

QString ScenarioTemplate::oldHebrewTemplateName()
{
    return "Hebrew script template";
}

void ScenarioTemplate::setIsNew()
{
    m_isDefault = false;
    m_name.prepend(QApplication::translate("BusinessLogic::ScenarioTemplate", "Copy of "));
    m_description.clear();
}

void ScenarioTemplate::saveToFile(const QString& _filePath) const
{
    QFile templateFile(_filePath);
    if (templateFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QXmlStreamWriter writer(&templateFile);
        writer.setAutoFormatting(true);
        writer.writeStartDocument();
        writer.writeStartElement("style");
        writer.writeAttribute("name", m_name);
        writer.writeAttribute("description", m_description);
        writer.writeAttribute("page_format", PageMetrics::stringFromPageSizeId(m_pageSizeId));
        writer.writeAttribute("page_margins", ::stringFromMargins(m_pageMargins));
        writer.writeAttribute("numbering_alignment", toString(m_numberingAlignment));
        foreach (const ScenarioBlockStyle& blockStyle, m_blockStyles.values()) {
            writer.writeStartElement("block");
            writer.writeAttribute("id", toString(blockStyle.type()));
            writer.writeAttribute("active", toString(blockStyle.isActive()));
            writer.writeAttribute("font_family", blockStyle.font().family());
            writer.writeAttribute("font_size", toString(blockStyle.font().pointSize()));
            writer.writeAttribute("bold", toString(blockStyle.font().bold()));
            writer.writeAttribute("italic", toString(blockStyle.font().italic()));
            writer.writeAttribute("underline", toString(blockStyle.font().underline()));
            writer.writeAttribute("uppercase", toString(blockStyle.font().capitalization()
                                                          == QFont::AllUppercase));
            writer.writeAttribute("alignment", toString(blockStyle.align()));
            writer.writeAttribute("top_space", toString(blockStyle.topSpace()));
            writer.writeAttribute("bottom_space", toString(blockStyle.bottomSpace()));
            writer.writeAttribute("left_margin", toString(blockStyle.leftMargin()));
            writer.writeAttribute("top_margin", toString(blockStyle.topMargin()));
            writer.writeAttribute("right_margin", toString(blockStyle.rightMargin()));
            writer.writeAttribute("bottom_margin", toString(blockStyle.bottomMargin()));
            writer.writeAttribute("line_spacing", toString(blockStyle.lineSpacing()));
            writer.writeAttribute("line_spacing_value", toString(blockStyle.lineSpacingValue()));
            writer.writeAttribute("prefix", blockStyle.prefix());
            writer.writeAttribute("postfix", blockStyle.postfix());
            writer.writeEndElement(); // block
        }
        writer.writeEndElement(); // style
        writer.writeEndDocument();

        templateFile.close();
    }
}

bool ScenarioTemplate::isDefault() const
{
    return m_isDefault;
}

ScenarioBlockStyle ScenarioTemplate::blockStyle(ScenarioBlockStyle::Type _forType) const
{
    return m_blockStyles.value(_forType);
}

BusinessLogic::ScenarioBlockStyle ScenarioTemplate::blockStyle(const QTextBlock& _forBlock) const
{
    return blockStyle(ScenarioBlockStyle::forBlock(_forBlock));
}

void ScenarioTemplate::setName(const QString& _name)
{
    if (m_name != _name) {
        m_name = _name;
    }
}

void ScenarioTemplate::setDescription(const QString& _description)
{
    if (m_description != _description) {
        m_description = _description;
    }
}

void ScenarioTemplate::setPageSizeId(QPageSize::PageSizeId _pageSizeId)
{
    if (m_pageSizeId != _pageSizeId) {
        m_pageSizeId = _pageSizeId;
    }
}

void ScenarioTemplate::setPageMargins(const QMarginsF& _pageMargins)
{
    if (m_pageMargins != _pageMargins) {
        m_pageMargins = _pageMargins;
    }
}

void ScenarioTemplate::setNumberingAlignment(Qt::Alignment _alignment)
{
    if (m_numberingAlignment != _alignment) {
        m_numberingAlignment = _alignment;
    }
}

void ScenarioTemplate::setBlockStyle(const BusinessLogic::ScenarioBlockStyle& _blockStyle)
{
    m_blockStyles.insert(_blockStyle.type(), _blockStyle);
}

void ScenarioTemplate::updateBlocksColors()
{
    //
    // Цветовая схема
    //
    const bool useDarkTheme =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                "application/use-dark-theme",
                DataStorageLayer::SettingsStorage::ApplicationSettings)
            .toInt();
    const QString colorSuffix = useDarkTheme ? "-dark" : "";

    //
    // Определим цвета
    //
    QColor mainTextColor =
            QColor(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/text-color" + colorSuffix,
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                );
    QColor noprintableTextColor =
            QColor(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/nonprintable-text-color" + colorSuffix,
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                );
    QColor folderTextColor =
            QColor(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/folder-text-color" + colorSuffix,
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                );
    QColor folderBackgroundColor =
            QColor(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/folder-background-color" + colorSuffix,
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                );

    //
    // Обновим цвета блоков
    //
    foreach (ScenarioBlockStyle::Type blockStyleType, m_blockStyles.keys()) {
        ScenarioBlockStyle& blockStyle = m_blockStyles[blockStyleType];
        switch (blockStyleType) {

            default: {
                blockStyle.setTextColor(mainTextColor);
                break;
            }

            case ScenarioBlockStyle::NoprintableText: {
                blockStyle.setTextColor(noprintableTextColor);
                break;
            }

            case ScenarioBlockStyle::FolderFooter:
            case ScenarioBlockStyle::FolderHeader: {
                blockStyle.setTextColor(folderTextColor);
                blockStyle.setBackgroundColor(folderBackgroundColor);
                break;
            }
        }
    }
}

#ifdef MOBILE_OS
void ScenarioTemplate::setFontSize(int _size)
{
    for (auto iter = m_blockStyles.begin(); iter != m_blockStyles.end(); ++iter) {
        QFont font = m_blockStyles[iter.key()].font();
        font.setPointSize(_size);
        m_blockStyles[iter.key()].setFont(font);
    }
}

void ScenarioTemplate::configureMargins(qreal _availableWidth)
{
    const QFont font = m_blockStyles.first().font();
    const int characterWidth = QFontMetrics(font).width("W");
    const int fullLineLength = 62;
    const int fullLineWidth = characterWidth * fullLineLength;

    //
    // Если в доступное пространство влезает больше текста, чем должно быть на одной строке
    // определим размер отступов от краёв
    //
    qreal leftDocumentMargin = PageMetrics::mmToPx(18);
    _availableWidth -= leftDocumentMargin;
    qreal rightDocumentMargin = PageMetrics::mmToPx(12);
    _availableWidth -= rightDocumentMargin;
    qreal marginDelta = 0;
    if (_availableWidth > fullLineWidth) {
        marginDelta = (_availableWidth - fullLineWidth) / 2;
        leftDocumentMargin += marginDelta;
        rightDocumentMargin += marginDelta;
    }
    for (auto iter = m_blockStyles.begin(); iter != m_blockStyles.end(); ++iter) {
        qreal leftMarginPercents = 0;
        qreal rightMarginPercents = 0;
        switch (iter.key()) {
            case ScenarioBlockStyle::Character: {
                leftMarginPercents = 0.33;
                rightMarginPercents = 0.035;
                break;
            }

            case ScenarioBlockStyle::Parenthetical: {
                leftMarginPercents = 0.23;
                rightMarginPercents = 0.335;
                break;
            }

            case ScenarioBlockStyle::Dialogue: {
                leftMarginPercents = 0.17;
                rightMarginPercents = 0.26;
                break;
            }

            case ScenarioBlockStyle::Lyrics: {
                leftMarginPercents = 0.17;
                break;
            }

            default: break;
        }

        //
        // Рассчитаем полные отступы
        //
        const qreal currentLineWidth = _availableWidth > fullLineWidth ? fullLineWidth : _availableWidth;
        const qreal leftMargin = currentLineWidth * leftMarginPercents;
        const qreal rightMargin = currentLineWidth * rightMarginPercents;
        m_blockStyles[iter.key()].setLeftMargin(PageMetrics::pxToMm(leftMargin));
        m_blockStyles[iter.key()].setRightMargin(PageMetrics::pxToMm(rightMargin));
    }

    m_pageMargins.setLeft(PageMetrics::pxToMm(leftDocumentMargin));
    m_pageMargins.setRight(PageMetrics::pxToMm(rightDocumentMargin));
}
#endif

ScenarioTemplate::ScenarioTemplate(const QString& _fromFile)
{
    load(_fromFile);
}

void ScenarioTemplate::load(const QString& _fromFile)
{
    QFile xmlData(_fromFile);
    if (xmlData.open(QIODevice::ReadOnly)) {
        QXmlStreamReader reader(&xmlData);

        //
        // Считываем данные в соответствии с заданным форматом
        //
        if (reader.readNextStartElement() && (reader.name() == "style"))
        {
            //
            // Считываем атрибуты шаблона
            //
            QXmlStreamAttributes templateAttributes = reader.attributes();
            m_isDefault = templateAttributes.value("default").toString() == "true";
            m_name = templateAttributes.value("name").toString();
            m_description = templateAttributes.value("description").toString();
            //
            // ... если это один из стандартных стилей преобразуем его название и описание
            //
            if (m_isDefault) {
                if (m_name == "Final Draft screenplay (A4)") {
                    m_name = finalDraftA4TemplateName();
                    m_description = finalDraftA4TemplateDescription();
                } else if (m_name == "Final Draft screenplay (Letter)") {
                    m_name = finalDraftLetterTemplateName();
                    m_description = finalDraftLetterTemplateDescription();
                } else if (m_name == "Russian screenplay") {
                    m_name = russianTemplateName();
                    m_description = russianTemplateDescription();
                } else if (m_name == "Russian screenplay with Courier Prime") {
                    m_name = russianTemplateWithCourierPrimeTemplateName();
                    m_description = russianTemplateWithCourierPrimeTemplateDescription();
                } else if (m_name == "Chinese screenplay") {
                    m_name = chineseTemplateName();
                    m_description = chineseTemplateDescription();
                } else if (m_name == "Hebrew screenplay") {
                    m_name = hebrewTemplateName();
                    m_description = hebrewTemplateDescription();
                } else if (m_name == "Arabic screenplay") {
                    m_name = arabicTemplateName();
                    m_description = arabicTemplateDescription();
                }
            }
            //
            m_pageSizeId = PageMetrics::pageSizeIdFromString(templateAttributes.value("page_format").toString());
            m_pageMargins = ::marginsFromString(templateAttributes.value("page_margins").toString());
            const QString numberingAlignment = templateAttributes.value("numbering_alignment").toString();
            if (!numberingAlignment.isEmpty()) {
                m_numberingAlignment = ::alignmentFromString(numberingAlignment);
            } else {
                m_numberingAlignment = Qt::AlignTop | Qt::AlignRight;
            }

            //
            // Считываем настройки оформления блоков текста
            //
            while (reader.readNextStartElement() && (reader.name() == "block"))
            {
                ScenarioBlockStyle blockStyle(reader.attributes());
                m_blockStyles.insert(blockStyle.type(), blockStyle);

                //
                // Если ещё не находимся в конце элемента, то остальное пропускаем
                //
                if (!reader.isEndElement())
                    reader.skipCurrentElement();
            }

            //
            // NOTE: Для файлов шаблона созданных в старых версиях программы, проверяем наличие
            //		 стиля для описания сцены, если его нет, то копируем из описания действия
            //
            if (!m_blockStyles.contains(ScenarioBlockStyle::SceneDescription)) {
                ScenarioBlockStyle sceneDescriptionStyle = m_blockStyles.value(ScenarioBlockStyle::Action);
                sceneDescriptionStyle.setType(ScenarioBlockStyle::SceneDescription);
                m_blockStyles.insert(sceneDescriptionStyle.type(), sceneDescriptionStyle);
            }

            //
            // Копируем стиль для теневых заголовков
            //
            if (!m_blockStyles.contains(ScenarioBlockStyle::SceneHeadingShadow)) {
                ScenarioBlockStyle sceneHeadingShadowStyle = m_blockStyles.value(ScenarioBlockStyle::SceneHeading);
                sceneHeadingShadowStyle.setType(ScenarioBlockStyle::SceneHeadingShadow);
                setBlockStyle(sceneHeadingShadowStyle);
            }

            //
            // Создаём стиль для блоков хранящих внутри себя таблицу разделающую страницу
            //
            if (!m_blockStyles.contains(ScenarioBlockStyle::PageSplitter)) {
                ScenarioBlockStyle pageSplitterStyle = m_blockStyles.value(ScenarioBlockStyle::Action);
                pageSplitterStyle.setType(ScenarioBlockStyle::PageSplitter);
                pageSplitterStyle.setTopMargin(0.0);
                pageSplitterStyle.setLeftMargin(0.0);
                pageSplitterStyle.setRightMargin(0.0);
                pageSplitterStyle.setBottomMargin(0.0);
                pageSplitterStyle.setCanModify(false);
                m_blockStyles.insert(pageSplitterStyle.type(), pageSplitterStyle);
            }
        }
    }
}

// ********
// ScenarioTemplateFacade

QStandardItemModel* ScenarioTemplateFacade::templatesList()
{
    init();

    return s_instance->m_templatesModel;
}

bool ScenarioTemplateFacade::containsTemplate(const QString& _templateName)
{
    init();

    return s_instance->m_templates.contains(_templateName);
}

ScenarioTemplate ScenarioTemplateFacade::getTemplate(const QString& _templateName)
{
    init();

    const QString currentTemplate =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                "scenario-editor/current-style",
                DataStorageLayer::SettingsStorage::ApplicationSettings);

    ScenarioTemplate result;
    if (_templateName.isEmpty()) {
        if (!currentTemplate.isEmpty()
            && s_instance->m_templates.contains(currentTemplate)) {
            result = s_instance->m_templates.value(currentTemplate);
        } else {
            result = s_instance->m_defaultTemplate;
        }
    } else {
        //
        // Передаём значением по-умолчанию стандартный шаблон, т.к. иногда, например при смене языка
        // может возникнуть ситуация с передачей стандартного шаблона на другом языке
        //
        result = s_instance->m_templates.value(_templateName, s_instance->m_defaultTemplate);
    }

    return result;
}

void ScenarioTemplateFacade::saveTemplate(const BusinessLogic::ScenarioTemplate& _template)
{
    init();

    //
    // Если такого шаблона ещё не было раньше, то добавляем строку в модель шаблонов
    //
    if (!containsTemplate(_template.name())) {
        QStandardItem* stylesRootItem = s_instance->m_templatesModel->invisibleRootItem();
        QList<QStandardItem*> templateRow;
        templateRow << new QStandardItem(_template.name());
        templateRow << new QStandardItem(_template.description());
        stylesRootItem->appendRow(templateRow);
    }
    //
    // А если был обновляем описание
    //
    else {
        foreach (QStandardItem* templateItem, s_instance->m_templatesModel->findItems(_template.name())) {
            s_instance->m_templatesModel->setData(
                s_instance->m_templatesModel->index(templateItem->index().row(), 1),
                _template.description());
        }
    }
    //
    // Добавляем/обновляем шаблон в библиотеке
    //
    s_instance->m_templates.insert(_template.name(), _template);


    //
    // Настроим путь к папке с шаблонами
    //
    const QString appDataFolderPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString templatesFolderPath = appDataFolderPath + QDir::separator() + "Styles";
    const QString templateFilePath =
            templatesFolderPath + QDir::separator()
            + _template.name() + "." + SCENARIO_TEMPLATE_FILE_EXTENSION;
    //
    // Сохраняем шаблон в файл
    //
    _template.saveToFile(templateFilePath);
}

bool ScenarioTemplateFacade::saveTemplate(const QString& _templateFilePath)
{
    init();

    //
    // Загружаем шаблон из файла
    //
    ScenarioTemplate newTemplate(_templateFilePath);

    //
    // Если загрузка произошла успешно, то добавляем его в библиотеку
    //
    bool templateSaved = false;
    if (!newTemplate.name().isEmpty()) {
        saveTemplate(newTemplate);
        templateSaved = true;
    }

    return templateSaved;
}

void ScenarioTemplateFacade::removeTemplate(const QString& _templateName)
{
    init();

    //
    // Удалим шаблон из библиотеки
    //
    s_instance->m_templates.remove(_templateName);
    foreach (QStandardItem* templateItem, s_instance->m_templatesModel->findItems(_templateName)) {
        s_instance->m_templatesModel->removeRow(templateItem->row());
    }

    //
    // Настроим путь к папке с шаблонами
    //
    const QString appDataFolderPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString templatesFolderPath = appDataFolderPath + QDir::separator() + "Styles";
    const QString templateFilePath =
            templatesFolderPath + QDir::separator()
            + _templateName + "." + SCENARIO_TEMPLATE_FILE_EXTENSION;
    //
    // Удалим файл с шаблоном
    //
    QFile::remove(templateFilePath);
}

void ScenarioTemplateFacade::updateTemplatesColors()
{
    init();

    s_instance->m_defaultTemplate.updateBlocksColors();
    foreach (const QString& templateName, s_instance->m_templates.keys()) {
        s_instance->m_templates[templateName].updateBlocksColors();
    }
}

ScenarioTemplateFacade::ScenarioTemplateFacade()
{
    //
    // Настроим путь к папке с шаблонами
    //
    const QString appDataFolderPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString templatesFolderPath = appDataFolderPath + QDir::separator() + "Styles";
    //
    // ... создаём папку для пользовательских файлов
    //
    QDir rootFolder = QDir::root();
    rootFolder.mkpath(templatesFolderPath);

    //
    // Обновим шаблон по умолчанию
    //
    auto updateDefaultTemplate = [templatesFolderPath] (const QString& _templateName) {
        const QString defaultTemplatePath =
                templatesFolderPath + QDir::separator() + _templateName + SCENARIO_TEMPLATE_FILE_EXTENSION;
        QFile defaultTemplateFile(defaultTemplatePath);
        if (defaultTemplateFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QFile defaultTemplateRcFile(":/Templates/Templates/" + _templateName + SCENARIO_TEMPLATE_FILE_EXTENSION);
            if (defaultTemplateRcFile.open(QIODevice::ReadOnly)) {
                defaultTemplateFile.write(defaultTemplateRcFile.readAll());
                defaultTemplateRcFile.close();
            }
            defaultTemplateFile.close();
        }

        return defaultTemplatePath;
    };

    QString defaultTemplatePath;
#ifndef MOBILE_OS
    defaultTemplatePath = updateDefaultTemplate("final_draft_a4.");
    updateDefaultTemplate("russian.");
    updateDefaultTemplate("russian_courier_prime.");
    updateDefaultTemplate("final_draft_letter.");
    updateDefaultTemplate("chinese.");
    updateDefaultTemplate("hebrew.");
    updateDefaultTemplate("arabic.");
#else
    defaultTemplatePath = updateDefaultTemplate("mobile.");
    updateDefaultTemplate("russian.");
    updateDefaultTemplate("russian_courier_prime.");
    updateDefaultTemplate("final_draft_letter.");
    updateDefaultTemplate("final_draft_a4.");
    updateDefaultTemplate("chinese.");
    updateDefaultTemplate("hebrew.");
    updateDefaultTemplate("arabic.");
#endif

    //
    // Удаляем файлы default.kitss и default_courier_prime.kitss от старых версий, когда в них хранились русские шаблоны
    //
    QFile::remove(templatesFolderPath + QDir::separator() + "default.kitss");
    QFile::remove(templatesFolderPath + QDir::separator() + "default_courier_prime.kitss");

    //
    // Загрузить шаблоны
    //
    QDir templatesDir(templatesFolderPath);
    foreach (const QFileInfo& templateFile, templatesDir.entryInfoList(QDir::Files)) {
        if (templateFile.suffix() == SCENARIO_TEMPLATE_FILE_EXTENSION) {
            ScenarioTemplate templateObj(templateFile.absoluteFilePath());
            if (!m_templates.contains(templateObj.name())) {
                m_templates.insert(templateObj.name(), templateObj);
            }
        }
    }
    //
    // ... шаблон по умолчанию
    //
    m_defaultTemplate = ScenarioTemplate(defaultTemplatePath);

    //
    // Настроим модель шаблонов
    //
    m_templatesModel = new QStandardItemModel;
    QStandardItem* rootItem = m_templatesModel->invisibleRootItem();
    foreach (const ScenarioTemplate& templateObj, m_templates.values()) {
        if (templateObj.name() == "Mobile") {
            continue;
        }

        QList<QStandardItem*> row;
        row << new QStandardItem(templateObj.name());
        row << new QStandardItem(templateObj.description());
        rootItem->appendRow(row);
    }
}

void ScenarioTemplateFacade::init()
{
    //
    // Если необходимо создаём одиночку
    //
    if (s_instance == 0) {
        s_instance = new ScenarioTemplateFacade;
    }
}

ScenarioTemplateFacade* ScenarioTemplateFacade::s_instance = 0;
