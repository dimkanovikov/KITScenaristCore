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
		QString textToCheck = _text.simplified();
		if (!textToCheck.isEmpty()) {
            QRegExp notWord("([^\\w,^\\'\\-])+");
            notWord.indexIn(textToCheck);
			//
			// Проверяем каждое слово
			//
            int wordPos = 0;
            int notWordPos = notWord.pos(0);
            for (wordPos = 0; wordPos < textToCheck.length(); wordPos = notWordPos + 1) {
				//
				// Убираем знаки препинания окружающие слово
				//
                notWord.indexIn(textToCheck, wordPos + 1);
                notWordPos = notWord.pos(0);
                if (notWordPos == -1) {
                    notWordPos = textToCheck.length();
                }
                QString wordToCheck = textToCheck.mid(wordPos, notWordPos - wordPos);
                QString wordWithoutPunct = wordToCheck.trimmed();
				while (!wordWithoutPunct.isEmpty()
					   && (wordWithoutPunct.at(0).isPunct()
						   || wordWithoutPunct.at(wordWithoutPunct.length()-1).isPunct())) {
					if (wordWithoutPunct.at(0).isPunct()) {
						wordWithoutPunct = wordWithoutPunct.mid(1);
					} else {
						wordWithoutPunct = wordWithoutPunct.left(wordWithoutPunct.length()-1);
					}
				}

				//
				// Проверяем слова длинной более одного символа
				//
				if (wordWithoutPunct.length() > 1) {
                    int positionInText = wordPos; //_text.indexOf(wordWithoutPunct, 0);
                    if (m_edited
                            && positionInText <= m_cursorPosition
                            && positionInText + wordWithoutPunct.length() > m_cursorPosition) {
                        continue;
                    }
                    //
					// Корректируем регистр слова
					//
					QString wordWithoutPunctInCorrectRegister =
							wordWithoutPunct[0] + wordWithoutPunct.mid(1).toLower();

					//
					// Если слово не прошло проверку
					//
					if (!m_spellChecker->spellCheckWord(wordWithoutPunctInCorrectRegister)) {
						const int wordWithoutPunctLength = wordWithoutPunct.length();
                        setFormat(positionInText, wordWithoutPunctLength, m_misspeledCharFormat);
					}
				}
			}
        }
    }
}
