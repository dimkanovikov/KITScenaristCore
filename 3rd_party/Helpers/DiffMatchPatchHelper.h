#ifndef DIFFMATCHPATCHHELPER
#define DIFFMATCHPATCHHELPER

#include "DiffMatchPatch.h"
#include "TextEditHelper.h"

#include <QDomDocument>
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QString>
#include <QRegularExpression>

namespace {
    /**
     * @brief Является ли строка тэгом
     */
    static bool isTag(const QString& _tag) {
        return !_tag.isEmpty() && _tag.startsWith("<") && _tag.endsWith(">");
    }

    /**
     * @brief Является ли тэг открывающим
     */
    static bool isOpenTag(const QString& _tag) {
        return isTag(_tag) && !_tag.contains("/");
    }

    /**
     * @brief Является ли тэг закрывающим
     */
    static bool isCloseTag(const QString& _tag) {
        return isTag(_tag) && _tag.contains("/");
    }

    /**
     * @brief Удалить все xml-тэги сценария
     */
    QString removeCommonXmlTagsForScenario(const QString& _xml) {
        QString result = _xml;
        result = result.remove("<?xml version=\"1.0\"?>\n");
        result = result.remove("<scenario version=\"1.0\">\n");
        result = result.remove("</scenario>\n");

        return result;
    }

    /**
     * @brief Удалить все xml-тэги сценария
     */
    QString removeXmlTagsForScenario(const QString& _xml, const QHash<QString,QString> _tagsMap) {
        QString result = _xml;
        result = removeCommonXmlTagsForScenario(result);

        result = result.remove("<scene_group>\n");
        result = result.remove("</scene_group>\n");
        result = result.remove("<folder>\n");
        result = result.remove("</folder>\n");
        result = result.remove("<reviews>\n");
        result = result.remove("</reviews>\n");
        result = result.remove("<reviews/>\n");
        result = result.remove("<review>\n");
        result = result.remove("</review>\n");
        result = result.remove("<review/>\n");
        result = result.remove("<review_comment>\n");
        result = result.remove("</review_comment>\n");
        result = result.remove("<review_comment/>\n");
        result = result.remove("<formatting>\n");
        result = result.remove("</formatting>\n");
        result = result.remove("<formatting/>\n");
        result = result.remove("<format>\n");
        result = result.remove("</format>\n");
        result = result.remove("<format/>\n");

        for (const QString& tag : _tagsMap.keys()) {
            //
            // Для CDATA не убираем перенос строки,
            // т.к. он должен учитываться за отдельный символ
            //
            const QString suffix = isTag(tag) ? "\n" : "";
            result = result.remove(tag + suffix);
        }

        auto cleanResult = [&result] (const QString& _pattern) {
            result = result.remove(QRegularExpression(_pattern, QRegularExpression::MultilineOption
                                                      | QRegularExpression::DotMatchesEverythingOption
                                                      | QRegularExpression::InvertedGreedinessOption));
        };
        cleanResult("<scene_heading(.*)>\n");
        cleanResult("<scene_group_header(.*)>\n");
        cleanResult("<folder_header(.*)>\n");
        cleanResult("<review(.*)>\n");
        cleanResult("<review_comment(.*)>\n");
        cleanResult("<format(.*)>\n");
        //
        cleanResult("<scene_characters(.*)>\n");
        cleanResult("<action(.*)>\n");
        cleanResult("<character(.*)>\n");
        cleanResult("<parenthetical(.*)>\n");
        cleanResult("<dialog(.*)>\n");
        cleanResult("<transition(.*)>\n");
        cleanResult("<note(.*)>\n");
        cleanResult("<title_header(.*)>\n");
        cleanResult("<title(.*)>\n");
        cleanResult("<noprintable_text(.*)>\n");
        cleanResult("<scene_description(.*)>\n");
        cleanResult("<undefined(.*)>\n");
        cleanResult("<lyrics(.*)>\n");

        result = TextEditHelper::removeXmlTags(result);
        result = TextEditHelper::fromHtmlEscaped(result);
        return result;
    }
}


/**
 * @brief Класс со вспомогательными функциями для сравнения xml-текстов
 */
class DiffMatchPatchHelper
{
public:
    /**
     * @brief Сформировать патч между двумя простыми текстами
     */
    static QString makePatch(const QString& _text1, const QString& _text2) {
        diff_match_patch dmp;
        return dmp.patch_toText(dmp.patch_make(_text1, _text2));
    }

    /**
     * @brief Сформировать патч между двумя xml-текстами
     */
    static QString makePatchXml(const QString& _xml1, const QString& _xml2) {
        return
                plainToXml(
                    makePatch(
                        xmlToPlain(_xml1),
                        xmlToPlain(_xml2)
                        )
                    );
    }

    /**
     * @brief Применить патч для простого текста
     */
    static QString applyPatch(const QString& _text, const QString& _patch) {
        diff_match_patch dmp;
        QList<Patch> patches = dmp.patch_fromText(_patch);
        return dmp.patch_apply(patches, _text).first;
    }

    /**
     * @brief Применить патч для xml-текста
     */
    static QString applyPatchXml(const QString& _xml, const QString& _patch) {
        return
                plainToXml(
                    applyPatch(
                        xmlToPlain(_xml),
                        xmlToPlain(_patch)
                        )
                    );
    }

    /**
     * @brief Изменение xml
     */
    class ChangeXml {
    public:
        /**
         * @brief Сам xml
         */
        QString xml;

        /**
         * @brief Позиция изменения
         */
        int plainPos = -1;

        /**
         * @brief Длина текста
         */
        int plainLength = -1;

        ChangeXml() = default;
        ChangeXml(const QString& _xml, const int _pos, const int _length = -1) :
            xml(_xml), plainPos(_pos), plainLength(_length)
        {
            if (_length == -1) {
                //
                // Считаем длину убирая xml-тэги и последний перевод строки
                //
                QString plain = removeXmlTagsForScenario(xml, tagsMap());
                if (plain.endsWith("\n")) {
                    plain.chop(1);
                }
                plainLength = plain.length();
            }

            //
            // Убираем тэги folder и scene_group, чтобы избавиться от несбалансированного xml
            //
            xml.remove("<folder>");
            xml.remove("</folder>");
            xml.remove("<scene_group>");
            xml.remove("</scene_group>");
        }

        /**
         * @brief Валидно ли изменение
         */
        bool isValid() const {
            return plainPos != -1 && plainLength != -1;
        }
    };

    /**
     * @brief Определить куски xml из документов, которые затрагивает данное изменение
     * @return Пара: 1) текст, который был изменён; 2) текст замены
     */
    static QPair<ChangeXml, ChangeXml> changedXml(const QString& _xml, const QString& _patch, bool _checkXml = false) {
        //
        // Применим патчи
        //
        const QString oldXmlPlain = xmlToPlain(_xml);
        const QString newXml = applyPatchXml(_xml, _patch);

        //
        // Если нужно, проверим валидность xml
        //
        if (_checkXml) {
            QDomDocument document;
            QString error;
            int line, column;
            const auto xmlForCheck = QString("<?xml version=\"1.0\"?><scenario>%1</scenario>").arg(newXml);
            const bool isXmlValid = document.setContent(xmlForCheck, &error, &line, &column);
            if (!isXmlValid) {
                qDebug() << Q_FUNC_INFO << error;
                qDebug() << Q_FUNC_INFO << "line:" << line << "column:" << column;
                qDebug() << qPrintable(xmlForCheck);
                return QPair<ChangeXml, ChangeXml>();
            }
        }

        const QString newXmlPlain = xmlToPlain(newXml);

        //
        // Формируем новый патч, он будет содержать корректные данные,
        // для текста сценария текущего пользователя
        //
        const QString newPatch = makePatch(oldXmlPlain, newXmlPlain);
        if (newPatch.isEmpty()) {
            return QPair<ChangeXml, ChangeXml>();
        }

        //
        // Разберём патчи на список
        //
        diff_match_patch dmp;
        QList<Patch> patches = dmp.patch_fromText(newPatch);

        //
        // Рассчитаем метрики для формирования xml для обновления
        //
        int oldStartPos = -1;
        int oldEndPos = -1;
        int oldDistance = 0;
        int newStartPos = -1;
        int newEndPos = -1;
        for (const Patch& patch : patches) {
            //
            // ... для старого
            //
            if (oldStartPos == -1
                || patch.start1 < oldStartPos) {
                oldStartPos = patch.start1;
            }
            if (oldEndPos == -1
                || oldEndPos < (patch.start1 + patch.length1 - oldDistance)) {
                oldEndPos = patch.start1 + patch.length1 - oldDistance;
            }
            oldDistance += patch.length2 - patch.length1;
            //
            // ... для нового
            //
            if (newStartPos == -1
                || patch.start2 < newStartPos) {
                newStartPos = patch.start2;
            }
            if (newEndPos == -1
                || newEndPos < (patch.start2 + patch.length2)) {
                newEndPos = patch.start2 + patch.length2;
            }
        }
        //
        // Для случая, когда текста остаётся ровно столько же, сколько и было
        //
        if (oldDistance == 0) {
            oldEndPos = newEndPos;
        }
        //
        // Отнимает один символ, т.к. в патче указан индекс символа начиная с 1
        //
        oldEndPos -= 1;
        newEndPos -= 1;


        //
        // Определим кусок xml из текущего документа для обновления
        //
        int oldStartPosForXml = oldStartPos;
        for (; oldStartPosForXml > 0; --oldStartPosForXml) {
            //
            // Идём до открывающего тега
            //
            if (isOpenTag(tagsMap().key(oldXmlPlain.at(oldStartPosForXml)))) {
                break;
            }
        }
        int oldEndPosForXml = oldEndPos;
        for (; oldEndPosForXml < oldXmlPlain.length(); ++oldEndPosForXml) {
            //
            // Идём до закрывающего тэга, он находится в конце строки
            //
            if (isCloseTag(tagsMap().key(oldXmlPlain.at(oldEndPosForXml)))) {
                ++oldEndPosForXml;
                break;
            }
        }
        const QString oldXmlForUpdate = oldXmlPlain.mid(oldStartPosForXml, oldEndPosForXml - oldStartPosForXml);


        //
        // Определим кусок из нового документа для обновления
        //
        int newStartPosForXml = newStartPos;
        for (; newStartPosForXml > 0; --newStartPosForXml) {
            //
            // Идём до открывающего тега
            //
            if (isOpenTag(tagsMap().key(newXmlPlain.at(newStartPosForXml)))) {
                break;
            }
        }
        int newEndPosForXml = newEndPos;
        for (; newEndPosForXml < newXmlPlain.length(); ++newEndPosForXml) {
            //
            // Идём до закрывающего тэга, он находится в конце строки
            //
            if (isCloseTag(tagsMap().key(newXmlPlain.at(newEndPosForXml)))) {
                ++newEndPosForXml;
                break;
            }
        }
        const QString newXmlForUpdate = newXmlPlain.mid(newStartPosForXml, newEndPosForXml - newStartPosForXml);


        //
        // Определим позиции xml в тексте без тэгов
        //
        const QString oldXmlPart = oldXmlPlain.left(oldStartPosForXml);
        const int oldXmlPartLength = oldXmlPart.length();
        const int oldPlainPartLength = ::removeXmlTagsForScenario(plainToXml(oldXmlPart), tagsMap()).length();
        int oldStartPosForPlain = oldStartPosForXml - (oldXmlPartLength - oldPlainPartLength);
        //
        const QString newXmlPart = newXmlPlain.left(newStartPosForXml);
        const int newXmlPartLength = newXmlPart.length();
        const int newPlainPartLength = ::removeXmlTagsForScenario(plainToXml(newXmlPart), tagsMap()).length();
        int newStartPosForPlain = newStartPosForXml - (newXmlPartLength - newPlainPartLength);


//            qDebug() << oldStartPosForXml
//                     << oldXmlPartLength
//                     << oldPlainPartLength
//                     << oldStartPosForPlain << "\n\n--------------------------\n"
//                     << oldXmlPart << "\n\n--------------------------\n";
//            qDebug() << qPrintable(oldXml) << "\n\n--------------------------\n"
//                     << qPrintable(newXml) << "\n\n--------------------------\n"
//                     << qPrintable(oldXmlForUpdate) << "\n\n--------------------------\n"
//                     << qPrintable(newXmlForUpdate) << "\n\n\n****\n\n\n";


        return QPair<ChangeXml, ChangeXml>(
                    ChangeXml(plainToXml(oldXmlForUpdate), oldStartPosForPlain),
                    ChangeXml(plainToXml(newXmlForUpdate), newStartPosForPlain)
                    );
    }

    /**
     * @brief Определить куски xml из документов, которые затрагивает данное изменение
     * @return Вектор пар <текст, который был изменён | текст замены>
     */
    static QVector<QPair<ChangeXml, ChangeXml>> changedXmlList(const QString& _xml, const QString& _patch) {
        //
        // Применим патчи
        //
        const QString oldXml = xmlToPlain(_xml);
        const QString newXml = xmlToPlain(applyPatchXml(_xml, _patch));

        //
        // Формируем новый патч, он будет содержать корректные данные,
        // для текста сценария текущего пользователя
        //
        const QString newPatch = makePatch(oldXml, newXml);

        QVector<QPair<ChangeXml, ChangeXml>> result;
        if (!newPatch.isEmpty()) {
            //
            // Разберём патчи на список
            //
            diff_match_patch dmp;
            QList<Patch> patches = dmp.patch_fromText(newPatch);

            //
            // Рассчитаем метрики для формирования xml для обновления
            //
            int patchesDelta = 0;
            for (const Patch& patch : patches) {
                int oldStartPos = patch.start1 - patchesDelta;
                int oldEndPos = patch.start1 + patch.length1 - patchesDelta;
                int oldDistance = patch.length2 - patch.length1;
                int newStartPos = patch.start2;
                int newEndPos = patch.start2 + patch.length2;
                for (const auto& diff : patch.diffs) {
                    if (diff.operation == EQUAL) {
                        if (diff == patch.diffs.first()) {
                            oldStartPos += diff.text.length();
                            newStartPos += diff.text.length();
                        } else if (diff == patch.diffs.last()) {
                            oldEndPos -= diff.text.length();
                            newEndPos -= diff.text.length();
                        }
                    }
                }
                patchesDelta += patch.length2 - patch.length1;

                //
                // Для случая, когда текста остаётся ровно столько же, сколько и было
                //
                if (oldDistance == 0) {
                    oldEndPos = newEndPos;
                }
                //
                // Отнимаем один символ, т.к. в патче указан индекс символа начиная с 1
                //
                --oldStartPos;
                --oldEndPos;
                --newStartPos;
                --newEndPos;

                //
                // Определим кусок xml из текущего документа для обновления
                //
                int oldStartPosForXml = oldStartPos;
                for (; oldStartPosForXml > 0; --oldStartPosForXml) {
                    //
                    // Идём до открывающего тега
                    //
                    if (isOpenTag(tagsMap().key(oldXml.at(oldStartPosForXml)))) {
                        break;
                    }
                }
                int oldEndPosForXml = oldEndPos;
                for (; oldEndPosForXml < oldXml.length(); ++oldEndPosForXml) {
                    //
                    // Идём до закрывающего тэга, он находится в конце строки
                    //
                    if (isCloseTag(tagsMap().key(oldXml.at(oldEndPosForXml)))) {
                        ++oldEndPosForXml;
                        break;
                    }
                }
                const QString oldXmlForUpdate = oldXml.mid(oldStartPosForXml, oldEndPosForXml - oldStartPosForXml);


                //
                // Определим кусок из нового документа для обновления
                //
                int newStartPosForXml = newStartPos;
                for (; newStartPosForXml > 0; --newStartPosForXml) {
                    //
                    // Идём до открывающего тега
                    //
                    if (isOpenTag(tagsMap().key(newXml.at(newStartPosForXml)))) {
                        break;
                    }
                }
                int newEndPosForXml = newEndPos;
                for (; newEndPosForXml < newXml.length(); ++newEndPosForXml) {
                    //
                    // Идём до закрывающего тэга, он находится в конце строки
                    //
                    if (isCloseTag(tagsMap().key(newXml.at(newEndPosForXml)))) {
                        ++newEndPosForXml;
                        break;
                    }
                }
                const QString newXmlForUpdate = newXml.mid(newStartPosForXml, newEndPosForXml - newStartPosForXml);


                //
                // Определим позиции xml в тексте без тэгов
                //
                const QString oldXmlPart = oldXml.left(oldStartPosForXml);
                const int oldXmlPartLength = oldXmlPart.length();
                const int oldPlainPartLength = ::removeXmlTagsForScenario(plainToXml(oldXmlPart), tagsMap()).length();
                int oldStartPosForPlain = oldStartPosForXml - (oldXmlPartLength - oldPlainPartLength);
                //
                const QString newXmlPart = newXml.left(newStartPosForXml);
                const int newXmlPartLength = newXmlPart.length();
                const int newPlainPartLength = ::removeXmlTagsForScenario(plainToXml(newXmlPart), tagsMap()).length();
                int newStartPosForPlain = newStartPosForXml - (newXmlPartLength - newPlainPartLength);


    //            qDebug() << oldStartPosForXml
    //                     << oldXmlPartLength
    //                     << oldPlainPartLength
    //                     << oldStartPosForPlain << "\n\n--------------------------\n"
    //                     << oldXmlPart << "\n\n--------------------------\n";
    //            qDebug() << qPrintable(oldXml) << "\n\n--------------------------\n"
    //                     << qPrintable(newXml) << "\n\n--------------------------\n"
    //                     << qPrintable(oldXmlForUpdate) << "\n\n--------------------------\n"
    //                     << qPrintable(newXmlForUpdate) << "\n\n\n****\n\n\n";


                result.append({ ChangeXml(plainToXml(oldXmlForUpdate), oldStartPosForPlain),
                                ChangeXml(plainToXml(newXmlForUpdate), newStartPosForPlain) });
            }
        }

        return result;
    }

private:
    /**
     * @brief Преобразовать xml в плоский текст, заменяя тэги спецсимволами
     */
    static QString xmlToPlain(const QString& _xml) {
        //
        // TODO: искать тэги и добавлять их в карту, чтобы ещё более корректной была замена
        //		 Может быть не самой лучшей идеей, если работают одновременно несколько авторов их
        //		 карты замен будут разными и тогда сценарии не сойдутся
        //
        QString plain = _xml;
        foreach (const QString& key, tagsMap().keys()) {
            plain.replace(key, tagsMap().value(key));
        }
        return ::removeCommonXmlTagsForScenario(plain);
    }

    /**
     * @brief Преобразовать плоский текст в xml, заменяя спецсимволы на тэги
     */
    static QString plainToXml(const QString& _plain) {
        QString xml = _plain;
        foreach (const QString& value, tagsMap().values()) {
            xml.replace(value, tagsMap().key(value));
        }
        return xml;
    }

    /**
     * @brief Добавить тэг и его закрывающий аналог в карту соответствий
     */
    static void addTag(const QString& _tag, QHash<QString, QString>& _tagsMap, int& _index) {
        //
        // Для карты соответсвия используем символы юникода, которые врядли будут использоваться
        //
        _tagsMap.insert("<" + _tag + ">", QChar(_index++));
        _tagsMap.insert("</" + _tag + ">", QChar(_index++));
    }

    /**
     * @brief Карта соответствий xml-тэгов и спецсимволов
     */
    static const QHash<QString,QString>& tagsMap() {
        static QHash<QString,QString> s_tagsMap;
        static int s_charIndex = 44032;
        if (s_tagsMap.isEmpty()) {
            //
            // WARNING: Добавлять новые теги, только в конец карты, ни в коем случае не в начало,
            //			или середину, иначе это порушит совместимость со всеми предыдущими патчами
            //
            addTag("scene_heading", s_tagsMap, s_charIndex);
            addTag("scene_characters", s_tagsMap, s_charIndex);
            addTag("action", s_tagsMap, s_charIndex);
            addTag("character", s_tagsMap, s_charIndex);
            addTag("parenthetical", s_tagsMap, s_charIndex);
            addTag("dialog", s_tagsMap, s_charIndex);
            addTag("transition", s_tagsMap, s_charIndex);
            addTag("note", s_tagsMap, s_charIndex);
            addTag("title_header", s_tagsMap, s_charIndex);
            addTag("title", s_tagsMap, s_charIndex);
            addTag("noprintable_text", s_tagsMap, s_charIndex);
            addTag("scene_group", s_tagsMap, s_charIndex);
            addTag("scene_group_header", s_tagsMap, s_charIndex);
            addTag("scene_group_footer", s_tagsMap, s_charIndex);
            addTag("folder", s_tagsMap, s_charIndex);
            addTag("folder_header", s_tagsMap, s_charIndex);
            addTag("folder_footer", s_tagsMap, s_charIndex);

            s_tagsMap.insert("<v><![CDATA[", QChar(s_charIndex++));
            s_tagsMap.insert("]]></v>", QChar(s_charIndex++));

            addTag("scene_description", s_tagsMap, s_charIndex);
            addTag("undefined", s_tagsMap, s_charIndex);
            addTag("lyrics", s_tagsMap, s_charIndex);

            addTag("review", s_tagsMap, s_charIndex);
            addTag("reviews", s_tagsMap, s_charIndex);

            /*
                ("<scene_heading>", "가")
                ("</scene_heading>", "각")
                ("<scene_characters>", "갂")
                ("</scene_characters>", "갃")
                ("<action>", "간")
                ("</action>", "갅")
                ("<character>", "갆")
                ("</character>", "갇")
                ("<parenthetical>", "갈")
                ("</parenthetical>", "갉")
                ("<dialog>", "갊")
                ("</dialog>", "갋")
                ("<transition>", "갌")
                ("</transition>", "갍")
                ("<note>", "갎"))
                ("</note>", "갏")
                ("<title_header>", "감")
                ("</title_header>", "갑")
                ("<title>", "값")
                ("</title>", "갓")
                ("<noprintable_text>", "갔")
                ("</noprintable_text>", "강")
                ("<scene_group>", "갖")
                ("</scene_group>", "갗")
                ("<scene_group_header>", "갘")
                ("</scene_group_header>", "같")
                ("<scene_group_footer>", "갚")
                ("</scene_group_footer>", "갛")
                ("<folder>", "개")
                ("</folder>", "객")
                ("<folder_header>", "갞")
                ("</folder_header>", "갟")
                ("<folder_footer>", "갠")
                ("</folder_footer>", "갡")

                ("<v><![CDATA[", "갢")
                ("]]></v>", "갣")

                ("<scene_description>", "갤")
                ("</scene_description>", "갥")
                ("<undefined>", "갦")
                ("</undefined>", "갧")
                ("<lirycs>", "갨")
                ("</lirycs>", "갩")

                ("<reviews>", "갬")
                ("</reviews>", "갭")
                ("</review>", "갫")
             */
        }
        return s_tagsMap;
    }
};

#endif // DIFFMATCHPATCHHELPER

