#include "ScenarioTextDocument.h"

#include "ScenarioReviewModel.h"
#include "ScenarioTextBlockInfo.h"
#include "ScenarioXml.h"
#include "ScriptBookmarksModel.h"
#include "ScriptTextCorrector.h"

#include <Domain/ScenarioChange.h>

#include <DataLayer/Database/DatabaseHelper.h>

#include <DataLayer/DataStorageLayer/ScenarioChangeStorage.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>
#include <DataLayer/DataStorageLayer/StorageFacade.h>

#include <3rd_party/Helpers/PasswordStorage.h>

#include <3rd_party/Widgets/QLightBoxWidget/qlightboxprogress.h>

#include <QApplication>
#include <QCryptographicHash>
#include <QDomDocument>
#include <QTextBlock>

//
// Для отладки работы с патчами
//
//#define PATCH_DEBUG
#ifdef PATCH_DEBUG
#include <QDebug>
#endif

using namespace BusinessLogic;
using DatabaseLayer::DatabaseHelper;

namespace {
/**
 * @brief Доступный размер изменений в редакторе
 */
const int MAX_UNDO_REDO_STACK_SIZE = 100;

/**
 * @brief Получить хэш текста
 */
static QByteArray textMd5Hash(const QString &_text)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(_text.toUtf8());
    return hash.result();
}

/**
 * @brief Сохранить изменение
 */
static Domain::ScenarioChange *saveChange(const QString &_undoPatch, const QString &_redoPatch)
{
    const QString username = DataStorageLayer::StorageFacade::userName();
    return DataStorageLayer::StorageFacade::scenarioChangeStorage()->append(username, _undoPatch,
                                                                            _redoPatch);
}

/**
 * @brief Разделить заданный xml-текст на блоки
 */
QStringList splitXml(const QString &_xml)
{
    QStringList xmlParts;
    int partStart = 0;
    bool isReadingStartTag = true;
    QString startTag;
    bool isReadingNextTag = false;
    QString nextTag;
    for (int currentPosition = 0; currentPosition < _xml.size(); ++currentPosition) {
        const auto &character = _xml[currentPosition];
        if (character == '<') {
            if (isReadingStartTag) {
                partStart = currentPosition;
                //
                // Очищаем от символов переноса строк после предыдущей части
                //
                startTag.clear();
            } else {
                isReadingNextTag = true;
                nextTag.clear();
            }
        } else if (character == '>') {
            if (isReadingStartTag) {
                //
                // Одинарный тэг
                //
                if (_xml[currentPosition - 1] == '/') {
                    xmlParts.append(_xml.mid(partStart, currentPosition - partStart + 1));
                    startTag.clear();
                }
                //
                // Игнорируем пустые завершаю тэги
                //
                else if (_xml.mid(partStart, currentPosition - partStart + 1).contains('/')) {
                    //                    if (xmlParts.size() == 1) {
                    //                        xmlParts.first() =
                    //                        QString("<%1><v><![CDATA[]]></v>%2</%1>").arg(startTag,
                    //                        xmlParts.first());
                    //                    }
                    xmlParts.append(startTag);
                    startTag.clear();
                }
                //
                // Преходим к считыванию контента
                //
                else {
                    if (startTag.contains(' ')) {
                        startTag = startTag.split(' ').first();
                    }
                    isReadingStartTag = false;
                }
            } else {
                isReadingNextTag = false;

                if (startTag == nextTag) {
                    xmlParts.append(_xml.mid(partStart, currentPosition - partStart + 1));
                    isReadingStartTag = true;
                    startTag.clear();
                }
            }
        } else if (character == '/') {
            //
            // Пропускаем символ завершения тэга, чтобы не считывать его
            //
        } else {
            if (isReadingStartTag) {
                startTag += character;
            } else if (isReadingNextTag) {
                nextTag += character;
            }
        }
    }

    //
    // Добавляем весь оставшийся разбитый xml
    //
    if (!startTag.simplified().isEmpty()) {
        xmlParts.append(_xml.mid(partStart));
    }

    return xmlParts;
}

/**
 * @brief Получить обычный текст из xml-текста
 */
QString plainText(const QString &_xml)
{
    const QString cdata = QLatin1String("<![CDATA[");
    const auto startPos = _xml.indexOf(cdata) + cdata.length();
    if (startPos == -1) {
        return {};
    }
    const auto endPos = _xml.indexOf("]]>", startPos);
    if (endPos == -1) {
        return {};
    }
    return _xml.mid(startPos, endPos - startPos);
}
} // namespace

void ScenarioTextDocument::updateBlockRevision(QTextBlock &_block)
{
    TextBlockInfo *blockInfo = dynamic_cast<TextBlockInfo *>(_block.userData());
    if (blockInfo != nullptr) {
        blockInfo->updateId();
    }

    _block.setRevision(_block.revision() + 1);
}

void ScenarioTextDocument::updateBlockRevision(QTextCursor &_cursor)
{
    QTextBlock block = _cursor.block();
    updateBlockRevision(block);
}

ScenarioTextDocument::ScenarioTextDocument(QObject *parent, ScenarioXml *_xmlHandler)
    : QTextDocument(parent)
    , m_xmlHandler(_xmlHandler)
    , m_isPatchApplyProcessed(false)
    , m_reviewModel(new ScenarioReviewModel(this))
    , m_bookmarksModel(new ScriptBookmarksModel(this))
    , m_outlineMode(false)
    , m_corrector(new ScriptTextCorrector(this))
{
    connect(this, &ScenarioTextDocument::contentsChange, this,
            &ScenarioTextDocument::updateBlocksIds);
    connect(m_reviewModel, &ScenarioReviewModel::reviewChanged, this,
            &ScenarioTextDocument::reviewChanged);
    connect(m_bookmarksModel, &ScriptBookmarksModel::modelChanged, this,
            &ScenarioTextDocument::bookmarksChanged);
}

bool ScenarioTextDocument::updateScenarioXml()
{
    if (!m_isPatchApplyProcessed) {
        const QString newScenarioXml = m_xmlHandler->scenarioToXml();
        const QByteArray newScenarioXmlHash = ::textMd5Hash(newScenarioXml);

        //
        // Если текущий текст сценария отличается от последнего сохранённого
        //
        if (newScenarioXmlHash != m_scenarioXmlHash) {
            m_scenarioXml = newScenarioXml;
            m_scenarioXmlHash = newScenarioXmlHash;
            return true;
        }
    }
    return false;
}

QString ScenarioTextDocument::scenarioXml() const
{
    return m_scenarioXml;
}

QByteArray ScenarioTextDocument::scenarioXmlHash() const
{
    return m_scenarioXmlHash;
}

void ScenarioTextDocument::load(const QString &_scenarioXml)
{
    //
    // Сбрасываем корректор
    //
    m_corrector->clear();

    //
    // Если xml не задан сформируем его пустой аналог
    //
    QString scenarioXml = _scenarioXml;
    if (scenarioXml.isEmpty()) {
        scenarioXml = m_xmlHandler->defaultTextXml();
    }

    //
    // Сохраняем текущий режим, для последующего восстановления
    // FIXME: Так нужно делать, чтобы в режиме поэпизодника не скакал курсор, если этот режим
    // активен
    //
    bool outlineMode = m_outlineMode;
    setOutlineMode(false);

    //
    // Загружаем проект
    //
    const bool remainLinkedData = true;
    m_xmlHandler->xmlToScenario(0, scenarioXml, remainLinkedData);
    m_scenarioXml = scenarioXml;
    m_scenarioXmlHash = ::textMd5Hash(scenarioXml);
    m_lastSavedScenarioXml = m_scenarioXml;
    m_lastSavedScenarioXmlHash = m_scenarioXmlHash;

    //
    // Восстанавливаем режим
    //
    setOutlineMode(outlineMode);

    m_undoStack.clear();
    m_redoStack.clear();

    emit redoAvailableChanged(false);

#ifdef PATCH_DEBUG
    foreach (DomainObject *obj,
             DataStorageLayer::StorageFacade::scenarioChangeStorage()->all()->toList()) {
        ScenarioChange *ch = dynamic_cast<ScenarioChange *>(obj);
        if (!ch->isDraft()) {
            m_undoStack.append(ch);
        }
    }
    foreach (DomainObject *obj,
             DataStorageLayer::StorageFacade::scenarioChangeStorage()->all()->toList()) {
        ScenarioChange *ch = dynamic_cast<ScenarioChange *>(obj);
        if (!ch->isDraft()) {
            m_redoStack.prepend(ch);
        }
    }
#endif
}

QString ScenarioTextDocument::mimeFromSelection(int _startPosition, int _endPosition) const
{
    QString mime;

    if (m_xmlHandler != 0) {
        //
        // Скорректируем позиции в случае необходимости
        //
        if (_startPosition > _endPosition) {
            qSwap(_startPosition, _endPosition);
        }

        mime = m_xmlHandler->scenarioToXml(_startPosition, _endPosition);
    }

    return mime;
}

void ScenarioTextDocument::insertFromMime(int _insertPosition, const QString &_mimeData)
{
    if (m_xmlHandler != 0) {
        m_xmlHandler->xmlToScenario(_insertPosition, _mimeData);
    }
}

int ScenarioTextDocument::applyPatch(const QString &_patch, bool _checkXml)
{
    updateScenarioXml();
    saveChanges();

    m_isPatchApplyProcessed = true;

    //
    // Определим xml для применения патча
    //
    const QString patchUncopressed = DatabaseHelper::uncompress(_patch);
    auto xmlsForUpdate
        = DiffMatchPatchHelper::changedXml(m_scenarioXml, patchUncopressed, _checkXml);
    if (!xmlsForUpdate.first.isValid() || !xmlsForUpdate.second.isValid()) {

#ifdef PATCH_DEBUG
        qDebug() << "===================================================================";
        qDebug() << "uncompress failed";
        qDebug() << "###################################################################";
        qDebug() << qUtf8Printable(xmlsForUpdate.first.xml);
        qDebug() << "###################################################################";
        qDebug() << qUtf8Printable(QByteArray::fromPercentEncoding(patchUncopressed.toUtf8()));
        qDebug() << "###################################################################";
        qDebug() << qUtf8Printable(xmlsForUpdate.second.xml);
#endif

        //
        // Патч не был применён
        //
        m_isPatchApplyProcessed = false;

        return -1;
    }

    //
    // Удалим одинаковые первые и последние xml-блоки
    //
    removeIdenticalParts(xmlsForUpdate);

    xmlsForUpdate.first.xml = ScenarioXml::makeMimeFromXml(xmlsForUpdate.first.xml);
    xmlsForUpdate.second.xml = ScenarioXml::makeMimeFromXml(xmlsForUpdate.second.xml);

    //
    // Выделяем текст сценария, соответствующий xml для обновления
    //
    QTextCursor cursor(this);
    cursor.beginEditBlock();
    //
    // Определим позицию курсора в соответствии с декорациями
    //
    int selectionStartPos = 0;
    int selectionEndPos = 0;
    //
    // Если начало и конец выделения равны и длина нулевая, значит произошли изменения
    // в редакторских заметках или форматировании предыдущего блока
    //
    if (xmlsForUpdate.first.plainPos == xmlsForUpdate.second.plainPos
        && xmlsForUpdate.first.plainLength == 0 && xmlsForUpdate.second.plainLength == 0) {
        const auto block = findBlock(m_corrector->correctedPosition(xmlsForUpdate.first.plainPos));
        selectionStartPos = block.previous().position();
        selectionEndPos = selectionStartPos;
    }
    //
    // В противном случае имеем обычную ситуацию
    //
    else {
        selectionStartPos = m_corrector->correctedPosition(xmlsForUpdate.first.plainPos);
        selectionEndPos = m_corrector->correctedPosition(xmlsForUpdate.first.plainPos
                                                         + xmlsForUpdate.first.plainLength);
    }
    //
    // ... собственно выделение
    //
    setCursorPosition(cursor, selectionStartPos);
    setCursorPosition(cursor, selectionEndPos, QTextCursor::KeepAnchor);

#ifdef PATCH_DEBUG
    qDebug() << "===================================================================";
    qDebug() << cursor.selectedText();
    qDebug() << "###################################################################";
    qDebug() << qUtf8Printable(xmlsForUpdate.first.xml);
    qDebug() << "###################################################################";
    qDebug() << qUtf8Printable(QByteArray::fromPercentEncoding(patchUncopressed.toUtf8()));
    qDebug() << "###################################################################";
    qDebug() << qUtf8Printable(xmlsForUpdate.second.xml);
#endif

    //
    // Замещаем его обновлённым
    //
    cursor.removeSelectedText();
    //
    // ... при этом не изменяем идентификаторов сцен, которые находятся в сценарии
    //
    const bool remainLinkedData = true;
    m_xmlHandler->xmlToScenario(selectionStartPos, xmlsForUpdate.second.xml, remainLinkedData);

    //
    // Запомним новый текст
    //
    m_scenarioXml = m_xmlHandler->scenarioToXml();
    m_scenarioXmlHash = ::textMd5Hash(m_scenarioXml);
    m_lastSavedScenarioXml = m_scenarioXml;
    m_lastSavedScenarioXmlHash = m_scenarioXmlHash;

    //
    // Патч применён
    //
    m_isPatchApplyProcessed = false;

    //
    // Завершаем изменение документа
    // если завершить изменение сразу после вставки текста, но до запоминания его хэша, то
    // может случиться проблема, что текущий хэш и хэш документа совпадут, что приведёт к
    // отсутствию одного патча в истории изменений
    //
    cursor.endEditBlock();

    return selectionStartPos;
}

void ScenarioTextDocument::applyPatches(const QList<QString> &_patches)
{
    m_isPatchApplyProcessed = true;

    //
    // Применяем патчи
    //
    QString newXml = m_scenarioXml;
    int currentIndex = 0, max = _patches.size();

#ifdef PATCH_DEBUG
    QString lastXml;
    bool needPrintXml = true;
#endif

    for (const QString &patch : _patches) {

#ifdef PATCH_DEBUG
        lastXml = newXml;
#endif

        const QString patchUncopressed = DatabaseHelper::uncompress(patch);
        newXml = DiffMatchPatchHelper::applyPatchXml(newXml, patchUncopressed);

#ifdef PATCH_DEBUG
        QDomDocument doc;
        QString error;
        int line = 0, column = 0;
        bool ok = doc.setContent(ScenarioXml::makeMimeFromXml(newXml), &error, &line, &column);
        if (!ok || lastXml == newXml) {
            qDebug() << "===================================================================";
            qDebug() << "***************" << currentIndex << "*****************";
            qDebug() << error << line << column;
            if (needPrintXml) {
                qDebug() << "********************************";
                qDebug() << qUtf8Printable(lastXml);
            }
            qDebug() << "********************************";
            qDebug() << qUtf8Printable(QByteArray::fromPercentEncoding(patchUncopressed.toUtf8()));
            if (needPrintXml) {
                qDebug() << "********************************";
                qDebug() << qUtf8Printable(newXml);
            }

            needPrintXml = false;
        } else {
            qDebug() << "===================================================================";
            qDebug() << "***************" << currentIndex << "*****************";
            qDebug() << qUtf8Printable(QByteArray::fromPercentEncoding(patchUncopressed.toUtf8()));
        }
#endif

        QLightBoxProgress::setProgressValue(++currentIndex, max);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    //
    // Начинаем изменение текста
    //
    QTextCursor cursor(this);
    cursor.beginEditBlock();

    //
    // Загружаем текст документа
    //
    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();
    //
    // ... при этом не изменяем идентификаторов сцен, которые находятся в сценарии
    //
    const bool remainLinkedData = true;
    m_xmlHandler->xmlToScenario(0, ScenarioXml::makeMimeFromXml(newXml), remainLinkedData);

    //
    // Запомним новый текст
    //
    m_scenarioXml = m_xmlHandler->scenarioToXml();
    m_scenarioXmlHash = ::textMd5Hash(m_scenarioXml);
    m_lastSavedScenarioXml = m_scenarioXml;
    m_lastSavedScenarioXmlHash = m_scenarioXmlHash;

    //
    // Патч применён
    //
    m_isPatchApplyProcessed = false;

    //
    // Завершаем изменение документа
    // если завершить изменение сразу после вставки текста, но до запоминания его хэша, то
    // может случиться проблема, что текущий хэш и хэш документа совпадут, что приведёт к
    // отсутствию одного патча в истории изменений
    //
    cursor.endEditBlock();
}

Domain::ScenarioChange *ScenarioTextDocument::saveChanges()
{
    Domain::ScenarioChange *change = 0;

    if (!m_isPatchApplyProcessed) {
        //
        // Если текущий текст сценария отличается от последнего сохранённого
        //
        if (m_scenarioXmlHash != m_lastSavedScenarioXmlHash) {
            //
            // Сформируем изменения
            //
            const QString undoPatch
                = DiffMatchPatchHelper::makePatchXml(m_scenarioXml, m_lastSavedScenarioXml);
            const QString undoPatchCompressed = DatabaseHelper::compress(undoPatch);
            const QString redoPatch
                = DiffMatchPatchHelper::makePatchXml(m_lastSavedScenarioXml, m_scenarioXml);
            const QString redoPatchCompressed = DatabaseHelper::compress(redoPatch);

            //
            // Сохраним изменения
            //
            change = ::saveChange(undoPatchCompressed, redoPatchCompressed);

            //
            // Запомним новый текст
            //
            m_lastSavedScenarioXml = m_scenarioXml;
            m_lastSavedScenarioXmlHash = m_scenarioXmlHash;

            //
            // Корректируем стеки последних действий
            //
            if (m_undoStack.size() == MAX_UNDO_REDO_STACK_SIZE) {
                m_undoStack.takeFirst();
            }
            m_undoStack.append(change);

            m_redoStack.clear();
            emit redoAvailableChanged(false);

#ifdef PATCH_DEBUG
            qDebug() << "-------------------------------------------------------------------";
            qDebug() << qUtf8Printable(QByteArray::fromPercentEncoding(undoPatch.toUtf8()));
            qDebug() << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$";
            qDebug() << qUtf8Printable(QByteArray::fromPercentEncoding(redoPatch.toUtf8()));
#endif
        }
    }

    return change;
}

int ScenarioTextDocument::undoReimpl(bool _forced)
{
#ifdef MOBILE_OS
    QApplication::inputMethod()->commit();
#endif

    saveChanges();

    int pos = -1;
    if (!m_undoStack.isEmpty()) {
        Domain::ScenarioChange *change = m_undoStack.takeLast();

#ifdef PATCH_DEBUG
        qDebug() << "*******************************************************************";
        qDebug() << change->uuid().toString() << change->user() << characterCount()
                 << change->datetime().toString("yyyy-MM-dd hh:mm:ss:zzz");
#endif

        //
        // Перенесём действие в список действий доступных к повторению
        //
        if (!_forced) {
            m_redoStack.append(change);
            emit redoAvailableChanged(true);
        }

        pos = applyPatch(change->undoPatch());

        //
        // Сохраним изменения
        //
        if (!_forced) {
            Domain::ScenarioChange *newChange
                = ::saveChange(change->redoPatch(), change->undoPatch());
            newChange->setIsDraft(change->isDraft());
        }
    }
    return pos;
}

void ScenarioTextDocument::addUndoChange(ScenarioChange *change)
{
    m_undoStack.append(change);
}

void ScenarioTextDocument::updateUndoStack()
{
    m_undoStack.clear();
    m_redoStack.clear();
    foreach (DomainObject *obj,
             DataStorageLayer::StorageFacade::scenarioChangeStorage()->all()->toList()) {
        ScenarioChange *ch = dynamic_cast<ScenarioChange *>(obj);
        if (!ch->isDraft()) {
            m_undoStack.append(ch);
        }
    }
}

int ScenarioTextDocument::redoReimpl()
{
#ifdef MOBILE_OS
    QApplication::inputMethod()->commit();
#endif

    int pos = -1;
    if (!m_redoStack.isEmpty()) {
        Domain::ScenarioChange *change = m_redoStack.takeLast();

#ifdef PATCH_DEBUG
        qDebug() << "*******************************************************************";
        qDebug() << change->uuid().toString() << change->user() << characterCount()
                 << change->datetime().toString("yyyy-MM-dd hh:mm:ss:zzz");
#endif

        m_undoStack.append(change);
        pos = applyPatch(change->redoPatch());

        //
        // Сохраним изменения
        //
        Domain::ScenarioChange *newChange = ::saveChange(change->undoPatch(), change->redoPatch());
        newChange->setIsDraft(change->isDraft());

        //
        // Если больше нельзя повторить отменённое действие, испускаем соответствующий сигнал
        //
        if (m_redoStack.isEmpty()) {
            emit redoAvailableChanged(false);
        }
    }

    return pos;
}

bool ScenarioTextDocument::isUndoAvailableReimpl() const
{
    return !m_undoStack.isEmpty();
}

bool ScenarioTextDocument::isRedoAvailableReimpl() const
{
    return !m_redoStack.isEmpty();
}

void ScenarioTextDocument::setCursorPosition(QTextCursor &_cursor, int _position,
                                             QTextCursor::MoveMode _moveMode)
{
    //
    // Нормальное позиционирование
    //
    if (_position >= 0 && _position < characterCount()) {
        _cursor.setPosition(_position, _moveMode);
    }
    //
    // Для отрицательного ни чего не делаем, оставляем курсор в нуле
    //
    else if (_position < 0) {
        _cursor.movePosition(QTextCursor::Start, _moveMode);
    }
    //
    // Для очень большого, просто помещаем в конец документа
    //
    else {
        _cursor.movePosition(QTextCursor::End, _moveMode);
    }
}

ScenarioReviewModel *ScenarioTextDocument::reviewModel() const
{
    return m_reviewModel;
}

ScriptBookmarksModel *ScenarioTextDocument::bookmarksModel() const
{
    return m_bookmarksModel;
}

bool ScenarioTextDocument::outlineMode() const
{
    return m_outlineMode;
}

void ScenarioTextDocument::setOutlineMode(bool _outlineMode)
{
    m_outlineMode = _outlineMode;

    //
    // Сформируем список типов блоков для отображения
    //
    QList<ScenarioBlockStyle::Type> visibleBlocksTypes = this->visibleBlocksTypes();

    //
    // Пробегаем документ и настраиваем видимые и невидимые блоки
    //
    QTextCursor cursor(this);
    while (!cursor.atEnd()) {
        QTextBlock block = cursor.block();
        block.setVisible(visibleBlocksTypes.contains(ScenarioBlockStyle::forBlock(block)));
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextBlock);
    }
}

QList<ScenarioBlockStyle::Type> ScenarioTextDocument::visibleBlocksTypes() const
{
    static QList<ScenarioBlockStyle::Type> s_outlineVisibleBlocksTypes
        = QList<ScenarioBlockStyle::Type>()
        << ScenarioBlockStyle::SceneHeading << ScenarioBlockStyle::SceneHeadingShadow
        << ScenarioBlockStyle::SceneCharacters << ScenarioBlockStyle::FolderHeader
        << ScenarioBlockStyle::FolderFooter << ScenarioBlockStyle::SceneDescription;

    static QList<ScenarioBlockStyle::Type> s_scenarioVisibleBlocksTypes
        = QList<ScenarioBlockStyle::Type>()
        << ScenarioBlockStyle::SceneHeading << ScenarioBlockStyle::SceneHeadingShadow
        << ScenarioBlockStyle::SceneCharacters << ScenarioBlockStyle::Action
        << ScenarioBlockStyle::Character << ScenarioBlockStyle::Dialogue
        << ScenarioBlockStyle::Parenthetical << ScenarioBlockStyle::TitleHeader
        << ScenarioBlockStyle::Title << ScenarioBlockStyle::Note << ScenarioBlockStyle::Transition
        << ScenarioBlockStyle::NoprintableText << ScenarioBlockStyle::FolderHeader
        << ScenarioBlockStyle::FolderFooter << ScenarioBlockStyle::Lyrics;

    return m_outlineMode ? s_outlineVisibleBlocksTypes : s_scenarioVisibleBlocksTypes;
}

void ScenarioTextDocument::setCorrectionOptions(bool _needToCorrectCharactersNames,
                                                bool _needToCorrectPageBreaks)
{
    m_corrector->setNeedToCorrectCharactersNames(_needToCorrectCharactersNames);
    m_corrector->setNeedToCorrectPageBreaks(_needToCorrectPageBreaks);
}

void ScenarioTextDocument::correct(int _position, int _charsRemoved, int _charsAdded)
{
    m_corrector->correct(_position, _charsRemoved, _charsAdded);
}

void ScenarioTextDocument::updateBlocksIds(int _position, int _charsRemoved, int _charsAdded)
{
    Q_UNUSED(_charsRemoved);

    auto block = findBlock(_position);
    while (block.isValid() && block.position() <= _position + _charsAdded) {
        if (auto blockInfo = dynamic_cast<TextBlockInfo *>(block.userData())) {
            blockInfo->updateId();
        }

        block = block.next();
    }
}

void ScenarioTextDocument::removeIdenticalParts(
    QPair<DiffMatchPatchHelper::ChangeXml, DiffMatchPatchHelper::ChangeXml> &_xmls)
{
    auto firstSplitted = splitXml(_xmls.first.xml);
    auto secondSplitted = splitXml(_xmls.second.xml);
    int posDelta = 0;
    int lengthDelta = 0;
    QString lastRemovedPreviousTag;

    int maxStepsCount = std::min(firstSplitted.size(), secondSplitted.size());
    for (int i = 0; i < maxStepsCount; ++i) {
        if (firstSplitted.at(i) == secondSplitted.at(i)) {
            lastRemovedPreviousTag.clear();
            const auto text = plainText(firstSplitted.at(i));
            //
            // Если впереди был блок без текста, сразу удаляем его
            //
            if (text.isNull()) {
                lastRemovedPreviousTag = firstSplitted[i];
                firstSplitted[i] = QString();
                secondSplitted[i] = QString();
            }
            //
            // В противном случае оставляем, чтобы накладывать патчи после него
            //
            else {
                firstSplitted[i].remove(text);
                firstSplitted[i].remove(QRegExp("<reviews>(.*)</reviews>"));
                firstSplitted[i].remove(QRegExp("<formatting>(.*)</formatting>"));
                secondSplitted[i].remove(text);
                secondSplitted[i].remove(QRegExp("<reviews>(.*)</reviews>"));
                secondSplitted[i].remove(QRegExp("<formatting>(.*)</formatting>"));

                const auto textLength = TextEditHelper::fromHtmlEscaped(text).length();
                posDelta += textLength;
                lengthDelta += textLength;
            }
            //
            // Если подряд идут несколько одинаковых блоков, то чистим предыдущий
            //
            if (i > 0) {
                const auto previousIndex = i - 1;
                if (!firstSplitted.at(previousIndex).isEmpty()) {
                    //
                    // + перенос строки
                    //
                    posDelta += 1;
                    lengthDelta += 1;
                }
                firstSplitted[previousIndex] = QString();
                secondSplitted[previousIndex] = QString();
            }
        } else {
            break;
        }
    }
    for (int i = 0; i < maxStepsCount; ++i) {
        if (firstSplitted.at(firstSplitted.size() - i - 1)
            == secondSplitted.at(secondSplitted.size() - i - 1)) {
            const auto text = plainText(firstSplitted.at(firstSplitted.size() - i - 1));
            if (text.isEmpty()) {
                break;
            }

            firstSplitted[firstSplitted.size() - i - 1] = QString();
            secondSplitted[secondSplitted.size() - i - 1] = QString();
            lengthDelta += TextEditHelper::fromHtmlEscaped(text).length();

            //
            // Если перед текстом есть ещё текст, то вычтем от длины перенос строки
            //
            if ((i + 1) < maxStepsCount) {
                if (!firstSplitted.at(firstSplitted.size() - i - 1 - 1).isEmpty()) {
                    ++lengthDelta; // + перенос строки
                }
            }
        } else {
            break;
        }
    }

    //
    // Очищаем всё, что не является xml'ем
    //
    auto clearInvalidXmlLines = [](QStringList& strings) {
        for (auto& string : strings) {
            if (!string.startsWith("<") && !string.endsWith(">")) {
                string = QString();
            }
        }
    };
    clearInvalidXmlLines(firstSplitted);
    clearInvalidXmlLines(secondSplitted);

    //
    // Кейс добавления ответа в комментарий
    //
    if (_xmls.first.plainLength < lengthDelta
        && _xmls.first.plainLength == _xmls.second.plainLength) {
        --lengthDelta;
    }

    //
    // Кейс с добавлением блока после блока с редакторскими правками
    //
    if (firstSplitted.join(QString()).isEmpty()
        && !lastRemovedPreviousTag.isEmpty()) {
        --posDelta;
        secondSplitted.prepend(QString("<%1><v><![CDATA[]]></v></%1>").arg(lastRemovedPreviousTag));
    }

    //
    // Кейс с удалением абзаца после редакторской правки
    //
    if (_xmls.second.plainLength < lengthDelta) {
        --posDelta;
        --lengthDelta;
    }

    _xmls.first.xml = firstSplitted.join(QString());
    _xmls.first.plainPos += posDelta;
    Q_ASSERT(_xmls.first.plainLength >= lengthDelta);
    _xmls.first.plainLength = _xmls.first.plainLength - lengthDelta;
    _xmls.second.xml = secondSplitted.join(QString());
    _xmls.second.plainPos += posDelta;
    Q_ASSERT(_xmls.second.plainLength >= lengthDelta);
    _xmls.second.plainLength = std::max(_xmls.second.plainLength - lengthDelta, 0);
}
