#include "InputView.h"

#include <QApplication>
#include <QInputMethod>
#include <QScreen>
#include <QTimer>


InputView::InputView(QWidget *_parent) :
    QWidget(_parent)
{
    connect(QApplication::inputMethod(), &QInputMethod::visibleChanged, this, &InputView::updateViewSize);
}

void InputView::prepare()
{
    QApplication::inputMethod()->show();
}

void InputView::finalize()
{
    QApplication::inputMethod()->hide();
}

void InputView::prepareImpl()
{
    //
    // Ни чего не делается в базовом классе
    //
}

void InputView::finalizeImpl()
{
    //
    // Ни чего не делается в базовом классе
    //
}

int InputView::additionalHeight() const
{
    return 0;
}

void InputView::updateViewSize()
{
    //
    // Возвращаем представление в исходный размер
    //
    setMaximumHeight(16777215);

    //
    // Если клавиатура закончила менять свой размер
    //
    bool needReupdate = true;
    if (!QApplication::inputMethod()->isAnimating()) {
        const bool keyboardIsVisible = QApplication::inputMethod()->isVisible();
        const QRectF keyboardRect = QApplication::inputMethod()->keyboardRectangle();
        //
        //  Если клавиатура должна быть видна и её размер удалось определить
        //
        if (keyboardIsVisible && keyboardRect.isValid()) {
            //
            // Скорректируем размер представления
            //
            const QScreen* screen = QApplication::primaryScreen();
            QSize newSize = screen->size();
            newSize.setHeight(newSize.height() - keyboardRect.height() - additionalHeight());
            setFixedHeight(newSize.height());

            needReupdate = false;
            qApp->setAutoSipEnabled(false);
        }
        //
        // Если клавиатура была скрыта, просто завершаем выполнение, т.к. размер
        // уже был восстановлен в самом начале выполнения функции
        //
        else if (!keyboardIsVisible) {
            needReupdate = false;
            qApp->setAutoSipEnabled(true);
        }
    }

    if (needReupdate) {
        QTimer::singleShot(30, this, &InputView::updateViewSize);
    }
}
