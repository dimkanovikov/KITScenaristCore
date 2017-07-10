#include "CompletableTextEdit.h"

#include <3rd_party/Helpers/StyleSheetHelper.h>
#include <3rd_party/Widgets/WAF/Animation/Animation.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QCompleter>
#include <QEvent>
#include <QListView>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QDebug>
namespace {
#ifdef MOBILE_OS
    /**
     * @brief Делегат для отрисовки текста по центру
     */
    class CenteredTextDelegate : public QStyledItemDelegate
    {
    public:
        CenteredTextDelegate(QObject* _parent) : QStyledItemDelegate(_parent) {}

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const{
            QStyleOptionViewItem myOption = _option;
            myOption.displayAlignment = Qt::AlignCenter;
            QStyledItemDelegate::paint(_painter, myOption, _index);
        }
    };

    const int COMPLETER_MAX_ITEMS = 3;
    const int COMPLETER_ITEM_HEIGHT = 40;

    /**
     * @brief Переопределяем комплитер, чтобы показывать список красиво
     */
	class MyCompleter : public QCompleter
	{
	public:
        explicit MyCompleter(QObject* _p = 0) :
            QCompleter(_p),
            m_popup(new QListView),
            m_popupDelegate(new CenteredTextDelegate(m_popup))
        {
            setPopup(m_popup);

            m_popup->setFocusPolicy(Qt::NoFocus);
            m_popup->installEventFilter(this);
            m_popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_popup->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            const QString styleSheet =
                    QString("QListView { show-decoration-selected: 0; }"
                            "QListView::item, QListView::item:selected { text-align: center; height: %1px; "
                            "border: none; border-bottom: %2px solid palette(highlighted-text); "
                            "background-color: palette(highlight); color: palette(highlighted-text); }")
                    .arg(StyleSheetHelper::dpToPx(COMPLETER_ITEM_HEIGHT))
                    .arg(StyleSheetHelper::dpToPx(1));
            m_popup->setStyleSheet(styleSheet);

            setMaxVisibleItems(COMPLETER_MAX_ITEMS);
        }

		/**
		 * @brief Переопределяется для отображения подсказки по глобальной координате
		 *		  левого верхнего угла области для отображения
		 */
		void completeReimpl(const QRect& _rect) {
            Q_UNUSED(_rect);

            m_popup->setWindowFlags(Qt::Widget);
            m_popup->setItemDelegate(m_popupDelegate);
            m_popup->setParent(qobject_cast<QWidget*>(parent()));
            m_popup->setFixedHeight(
                        qMin(completionCount(),
                             COMPLETER_MAX_ITEMS) * StyleSheetHelper::dpToPx(COMPLETER_ITEM_HEIGHT));
            m_popup->setFixedWidth(m_popup->parentWidget()->width());
            WAF::Animation::sideSlideIn(m_popup, WAF::TopSide, false);
		}

    protected:
        /**
         * @brief Переопределяем, чтобы скрывать декорации, во время закрытия попапа
         */
        bool eventFilter(QObject* _watched, QEvent* _event) {
            if (_watched == m_popup
                && m_popup->parentWidget() != nullptr
                && _event->type() == QEvent::Hide) {
                WAF::Animation::sideSlideOut(m_popup, WAF::TopSide, false);
            }
            return QCompleter::eventFilter(_watched, _event);
        }

    private:
        /**
         * @brief Виджет со списком автоподстановки
         */
        QListView* m_popup = nullptr;

        /**
         * @brief Делегат для отрисовки элементов списка
         */
        QStyledItemDelegate* m_popupDelegate = nullptr;
	};
#else
    class MyCompleter : public QCompleter
    {
    public:
        explicit MyCompleter(QObject* _p = 0) : QCompleter(_p) {}

        /**
         * @brief Переопределяется для отображения подсказки по глобальной координате
         *		  левого верхнего угла области для отображения
         */
        void completeReimpl(const QRect& _rect) {
            complete(_rect);
            popup()->move(_rect.topLeft());
        }
    };
#endif
}


CompletableTextEdit::CompletableTextEdit(QWidget* _parent) :
    SpellCheckTextEdit(_parent),
    m_useCompleter(true),
    m_completer(new MyCompleter(this))
{
	m_completer->setWidget(this);
	connect(m_completer, SIGNAL(activated(QString)), this, SLOT(applyCompletion(QString)));
}

void CompletableTextEdit::setUseCompleter(bool _use)
{
	if (m_useCompleter != _use) {
		m_useCompleter = _use;
	}
}

QCompleter* CompletableTextEdit::completer() const
{
	return m_completer;
}

bool CompletableTextEdit::isCompleterVisible() const
{
	return m_completer->popup()->isVisible();
}

bool CompletableTextEdit::complete(QAbstractItemModel* _model, const QString& _completionPrefix)
{
	bool success = false;

	if (m_useCompleter && canComplete()) {
		if (_model != 0) {
			//
			// Настроим завершателя, если необходимо
			//
			bool settedNewModel = m_completer->model() != _model;
			bool oldModelWasChanged = false;
			if (!settedNewModel
				&& _model != 0) {
				oldModelWasChanged = m_completer->model()->rowCount() == _model->rowCount();
			}

			if (settedNewModel
				|| oldModelWasChanged) {
				m_completer->setModel(_model);
				m_completer->setModelSorting(QCompleter::UnsortedModel);
				m_completer->setCaseSensitivity(Qt::CaseInsensitive);
			}
			m_completer->setCompletionPrefix(_completionPrefix);

			//
			// Если в модели для дополнения есть элементы
			//
			bool hasCompletions = m_completer->completionModel()->rowCount() > 0;
			bool alreadyComplete = _completionPrefix.toLower().endsWith(m_completer->currentCompletion().toLower());

			if (hasCompletions
				&& !alreadyComplete) {
				m_completer->popup()->setCurrentIndex(
							m_completer->completionModel()->index(0, 0));

				//
				// ... отобразим завершателя
				//
				QRect rect = cursorRect();
				rect.moveLeft(rect.left() + verticalScrollBar()->width() + viewportMargins().left());
				rect.moveTop(rect.top() + QFontMetricsF(currentCharFormat().font()).height());
				rect.setWidth(
							m_completer->popup()->sizeHintForColumn(0)
							+ m_completer->popup()->verticalScrollBar()->sizeHint().width());

				MyCompleter* myCompleter = static_cast<MyCompleter*>(m_completer);
				myCompleter->completeReimpl(rect);
				emit popupShowed();

				success = true;
			}
		}

		if (!success) {
			//
			// ... скроем, если был отображён
			//
			closeCompleter();
		}
	}

	return success;
}

void CompletableTextEdit::applyCompletion()
{
	if (isCompleterVisible()) {
		//
		// Получим выбранный из списка дополнений элемент
		//
		QModelIndex currentIndex = m_completer->popup()->currentIndex();
		QString completion = m_completer->popup()->model()->data(currentIndex).toString();

		applyCompletion(completion);

		closeCompleter();
	}
}

void CompletableTextEdit::applyCompletion(const QString& _completion)
{
	//
	// Вставим дополнение в текст
	//
	int completionStartFrom = m_completer->completionPrefix().length();
	QString textToInsert = _completion.mid(completionStartFrom);

	while (!textCursor().atBlockEnd()) {
		QTextCursor cursor = textCursor();
		cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
		if (cursor.selectedText().endsWith(m_completer->completionPrefix(), Qt::CaseInsensitive)) {
			break;
		}
		moveCursor(QTextCursor::NextCharacter);
	}

	textCursor().insertText(textToInsert);
}

void CompletableTextEdit::closeCompleter()
{
	m_completer->popup()->hide();
}

bool CompletableTextEdit::canComplete() const
{
	return true;
}
