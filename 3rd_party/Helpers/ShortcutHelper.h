#ifndef SHORTCUTHELPER
#define SHORTCUTHELPER

#include <QKeySequence>

#include <3rd_party/Helpers/TextUtils.h>


class ShortcutHelper
{
public:
    /**
     * @brief Сформировать платформозависимый шорткат
     */
    static QString makeShortcut(const QString& _shortcut) {
        return QKeySequence(_shortcut).toString(QKeySequence::NativeText);
    }

    /**
     * @brief Сформиовать платформозависимую подсказку
     */
    /** @{ */
    static QString makeToolTip(const QString& _text, const QString& _shortcut) {
        return makeToolTipImpl(_text, makeShortcut(_shortcut));
    }
    static QString makeToolTip(const QString _text, const QKeySequence& _shortcut) {
        return makeToolTipImpl(_text, _shortcut.toString(QKeySequence::NativeText));
    }
    /** @} */

private:
    /**
     * @brief Сформиовать платформозависимую подсказку
     */
    /** @{ */
    static QString makeToolTipImpl(const QString& _text, const QString& _shortcut) {
        return QString("%1 %2").arg(_text).arg(TextUtils::directedText(_shortcut, '(', ')'));
    }
};

#endif // SHORTCUTHELPER

