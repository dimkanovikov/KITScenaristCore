#ifndef TEXTUTILS_H
#define TEXTUTILS_H

#include <QFontMetrics>
#include <QPainter>
#include <QTextLayout>
#include <QTextOption>


class TextUtils
{
public:
    /**
     * @brief Обернуть строку специальными символами, чтобы избавиться от проблемы
     *        с QString("[%1]").arg(text) при вставке RTL-текста
     */
    static QString directedText(const QString& _text, const QChar& _startMark, const QChar& _endMark)
    {
        // mark (LRM/RLM) and embedding (LRE/RLE) directions
        const QString mDir = _text.isRightToLeft() ? QChar(0x200F) : QChar(0x200E);
        const QString eDir = _text.isRightToLeft() ? QChar(0x202B) : QChar(0x202A);
        return (_startMark.isNull() ? QString() : mDir + QString(_startMark))
                + eDir + _text
                + (_endMark.isNull() ? QString() : mDir + QString(_endMark));
    }

    /**
     * Возвращает минимальный прямоугольник, в который помещается текст.
     * @param text Текст
     * @param font Шрифт, которым рисуется текст
     * @param width Ширина фигуры (она останется неизменной)
     * @param option Параметры отображения
     * @return Размер прямоугольника.
     */
    static QSizeF textRect(const QString &text, const QFont &font, int width, const QTextOption &option)
    {
        QTextLayout textLayout(text, font);
        QFontMetricsF metrics(font);
        int leading = metrics.leading();
        qreal height = 0;
        textLayout.setTextOption(option);
        textLayout.beginLayout();
        int length = 0;
        while(true)
        {
            QTextLine line = textLayout.createLine();
            if (!line.isValid()) break;
            line.setLineWidth(width);
            height += leading + line.height();
            length += line.textLength();
        }
        textLayout.endLayout();
        height += metrics.height() * text.count("\n");
        return QSizeF(width, height);
    }

    /**
     * Возвращает сокращенный текст, такой, чтобы не превышал указанных размеров при рендеринге.
     * @param text Текст, который нужно сократить.
     * @param font Шрифт, которым выводится текст.
     * @param sz Максимальные размеры текста.
     * @param option Всякие опции для текста. Будут использованы в QTextLayout.
     * @returns Сокращенный текст (с многоточием на конце).
     *          Если весь помещается - то весь и будет возвращен.
     */
    static QString elidedText(const QString &text, const QFont &font, const QSizeF &sz, const QTextOption &option)
    {
        //
        // Определим сколько строк могут вместиться в заданную область
        //
        const QFontMetricsF metrics(font);
        const int linesInRect = sz.height() / metrics.lineSpacing();
        if (linesInRect == 1) {
            return metrics.elidedText(text, Qt::ElideRight, sz.width());
        }

        //
        // Определяем вмещающийся текст (примерно)
        //
        QTextLayout textLayout(text);
        textLayout.setFont(font);
        textLayout.setTextOption(option);
        qreal widthUsed = 0.;
        int lineCount = 0;
        textLayout.beginLayout();
        while (++lineCount < linesInRect) {
            QTextLine line = textLayout.createLine();
            if (!line.isValid()) {
                break;
            }

            line.setLineWidth(sz.width());
            widthUsed += line.width();

        }
        textLayout.endLayout();
        if (lineCount < linesInRect) {
            return metrics.elidedText(text, Qt::ElideRight, widthUsed);
        }

        return text;


//        /*
//         Сначала определим длину текста, который умещается в прямоугольнике
//         Для этого найдем количество строк, которые там умещаются (+ еще переводы!)
//         И затем суммируем их длины
//        */
//        QTextLayout textLayout(text, font);
//        QFontMetricsF metrics(font);
//        int leading = metrics.leading();
//        qreal height = 0;
//        textLayout.setTextOption(option);
//        textLayout.beginLayout();
//        int length = 0;
//        while(true)
//        {
//            QTextLine line = textLayout.createLine();
//            if (!line.isValid()) break;
//            line.setLineWidth(sz.width());
//            height += leading + line.height();
//            if (height>sz.height()) break;
//            length += line.textLength();
//        }
//        textLayout.endLayout();
//        QString str = text.left(length); // Это наша искомая строка, но без многоточия

//        /*
//         Теперь нужно найти, сколько текста поместится вместе с многоточием.
//         Следует учитывать, что, если текст можно отобразить полностью,
//         многоточия не требуется.
//        */
//        QRectF shaperect = QRectF(0,0,sz.width(),sz.height());
//        QRectF shaperecth = QRectF(0,0,sz.width(),sz.height()*2); // очень высокий, чтобы можно было узнать реальную высоту текста.
//        while(true)
//        {
//            // мало ли, вдруг зацикливание
//            if (str.length()==0) break;

//            // Выполняем цикл, пока текст не помещается в прямоугольник rect
//            QRectF rect = metrics.boundingRect(
//                              shaperecth,
//                              Qt::AlignLeft | Qt::TextWordWrap,
//                              str + (str.length()==text.length()? "" : "..."));
//            if (shaperect.contains(rect))
//                break;

//            // Отрезаем один символ и снова будем пытаться уместить его
//            str = str.left(str.length()-1);
//        }

        // собственно вот! ради чего все затевалось
//        return str + (str.length()==text.length()? "" : "..."); // и так сойдет
    }

    static void drawTextInRegion(QPainter* _painter, const QString& _text, const QRegion& _region, const QTextOption& _option)
    {
        auto calcTextInRect = [_painter, _text, _option] (const QRect& _rect) {
            QTextLayout textLayout(_text, _painter->font());
            QFontMetricsF metrics(_painter->font());
            int leading = metrics.leading();
            qreal height = 0;
            textLayout.setTextOption(_option);
            textLayout.beginLayout();
            int textEnd = -1;
            int lastLineTop = 0;
            while(true)
            {
                QTextLine line = textLayout.createLine();
                if (!line.isValid()) {
                    break;
                }
                if (lastLineTop + metrics.lineSpacing() > _rect.bottom()) {
                    break;;
                }

                line.setLineWidth(_rect.width());
                height += leading + line.height();
                textEnd += line.textStart() + line.textLength();
                lastLineTop += metrics.lineSpacing();
            }
            return textEnd;
        };

        int lastTextPos = 0;
        for (auto rect : _region) {
            const int startText = lastTextPos;
            lastTextPos = calcTextInRect(rect);
            if (lastTextPos == -1) {
                break;
            }
            _painter->drawText(rect, _text.mid(startText, lastTextPos));
        }
    }
};

#endif // TEXTUTILS_H
