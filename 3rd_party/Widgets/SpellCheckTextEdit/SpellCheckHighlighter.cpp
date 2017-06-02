#include "SpellCheckHighlighter.h"
#include "SpellChecker.h"

#include <QTextDocument>


SpellCheckHighlighter::SpellCheckHighlighter(QTextDocument* _parent, SpellChecker* _checker) :
	SyntaxHighlighter(_parent),
	m_spellChecker(_checker),
    m_useSpellChecker(true)
{
	Q_ASSERT(_checker);

	//
	// Настроим стиль выделения текста не прошедшего проверку
	//
	m_misspeledCharFormat.setUnderlineStyle(QTextCharFormat::DashUnderline);
	m_misspeledCharFormat.setUnderlineColor(Qt::red);
}

void SpellCheckHighlighter::setSpellChecker(SpellChecker* _checker)
{
	m_spellChecker = _checker;
	rehighlight();
}

void SpellCheckHighlighter::setUseSpellChecker(bool _use)
{
	if (m_useSpellChecker != _use) {
		m_useSpellChecker = _use;

		//
		// Если документ создан и не пуст, перепроверить его
		//
		if (document() != 0 && !document()->isEmpty()) {
			rehighlight();
		}
	}
}

bool SpellCheckHighlighter::useSpellChecker() const
{
    return m_useSpellChecker;
}

void SpellCheckHighlighter::setCursorPosition(int _position)
{
    if (m_cursorPosition != _position) {
        m_edited = false;
    }

    m_cursorPosition = _position;
}

void SpellCheckHighlighter::highlightBlock(const QString& _text)
{
	if (m_useSpellChecker) {
		//
		// Убираем пустоты из проверяемого текста
		//
        if (!_text.isEmpty()) {
            QRegExp notWord("[^\\w'-]+");
            notWord.indexIn(_text);
			//
			// Проверяем каждое слово
			//
            int wordPos = 0;
            int notWordPos = notWord.pos(0);
            for (wordPos = 0; wordPos < _text.length(); wordPos = notWordPos + 1) {
                //
                // Получим окончание слова
                //
                notWord.indexIn(_text, wordPos);
                notWordPos = notWord.pos(0);
                if (notWordPos == -1) {
                    notWordPos = _text.length();
                }

                //
                // Получим само слово
                //
                const QString wordToCheck = _text.mid(wordPos, notWordPos - wordPos);

				//
				// Проверяем слова длинной более одного символа
				//
                if (wordToCheck.length() > 1) {
                    int positionInText = wordPos;
                    //
                    // Не проверяем слово, которое сейчас пишется
                    if (m_edited
                            && positionInText <= m_cursorPosition
                            && positionInText + wordToCheck.length() > m_cursorPosition) {
                        continue;
                    }

					//
					// Если слово не прошло проверку
					//
                    if (!m_spellChecker->spellCheckWord(wordToCheck)) {
                        const int wordLength = wordToCheck.length();
                        setFormat(positionInText, wordLength, m_misspeledCharFormat);
					}
				}
			}
        }
    }
}
