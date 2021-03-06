#ifndef SPELLCHECKTEXTEDIT_H
#define SPELLCHECKTEXTEDIT_H

#include <3rd_party/Widgets/PagesTextEdit/PageTextEdit.h>
#include <QTextBlock>

#include "SpellChecker.h"

class SpellCheckHighlighter;


/**
 * @brief Класс текстового редактора с проверкой орфографии
 */
class SpellCheckTextEdit : public PageTextEdit
{
	Q_OBJECT

public:
	explicit SpellCheckTextEdit(QWidget* _parent = 0);

public slots:
	/**
	 * @brief Включить/выключить проверку правописания
	 */
	void setUseSpellChecker(bool _use);

	/**
	 * @brief Используется ли проверка орфографии
	 */
	bool useSpellChecker() const;

	/**
	 * @brief Установить язык для проверки орфографии
	 * @param Язык
	 */
	void setSpellCheckLanguage(SpellChecker::Language _language);

	/**
	 * @brief Получить язык проверки орфографии
	 */
	SpellChecker::Language spellCheckLanguage() const;

	/**
	 * @brief Создать контекстное меню
	 * @note Управление памятью передаётся вызывающему метод
	 */
	virtual QMenu* createContextMenu(const QPoint& _pos, QWidget* _parent = 0);

    /**
     * @brief Переопределяем для очистки собственных параметров, перед очисткой в  базовом классе
     */
    void clear() override;

protected:
	/**
	 * @brief Получить путь к файлу с пользовательским словарём
	 * @return Путь к файлу со словарём
	 *
	 * @note В текущей реализации возвращается пустая строка.
	 */
	virtual QString userDictionaryfile() const;

	/**
	 * @brief Переопределено для добавления действий связанных с проверкой орфографии
	 */
    void contextMenuEvent(QContextMenuEvent* _event) override;

	/**
	 * @brief Пересоздание подсвечивающего класса
	 */
	void setHighlighterDocument(QTextDocument* _document);

private slots:

	/**
	 * @brief Игнорировать слово под курсором
	 */
	void aboutIgnoreWord() const;

	/**
	 * @brief Добавить слово под курсором в пользовательский словарь
	 */
	void aboutAddWordToUserDictionary() const;

	/**
	 * @brief Заменить слово под курсором на выбранный вариант из предложенных
	 */
	void aboutReplaceWordOnSuggestion();

    /**
     * @brief Сменилась позиция курсора
     */
    void rehighlighWithNewCursor();

private:
	/**
	 * @brief Найти слово в позиции
	 * @param Позиция в тексте
	 * @return Слово, находящееся в данной позиции
	 */
	QString wordOnPosition(const QPoint& _position) const;

    /**
     * @brief Удаляет пунктуацию в слове
     */
    QString removePunctutaion(const QString& _word) const;

    /**
     * @brief Перемещает курсор в начало слова (с учетом - и ')
     */
    QTextCursor moveCursorToStartWord(QTextCursor cursor) const;

    /**
     * @brief Перемещает курсор в конец слова (с учетом - и ')
     */
    QTextCursor moveCursorToEndWord(QTextCursor cursor) const;

private:
	/**
	 * @brief Проверяющий орфографию
	 */
	SpellChecker* m_spellChecker;

	/**
	 * @brief Подсвечивающий орфографические ошибки
	 */
	SpellCheckHighlighter* m_spellCheckHighlighter;

	/**
	 * @brief Действия для слова не прошедшего проверку орфографии
	 */
	/** @{ */
	QAction* m_ignoreWordAction;
	QAction* m_addWordToUserDictionaryAction;
	QList<QAction*> m_suggestionsActions;
	/** @} */

	/**
	 * @brief Последняя позиция курсора, при открытии контекстного меню
	 */
	QPoint m_lastCursorPosition;

    /**
     * @brief Предыдущий блок, на котором был курсор
     */
    QTextBlock m_prevBlock;
};

#endif // SPELLCHECKTEXTEDIT_H
