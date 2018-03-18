#ifndef COLOREDTOOLBUTTON_H
#define COLOREDTOOLBUTTON_H

#include <QToolButton>

class ColorsPane;
class SlidingPanel;


/**
 * @brief Кнопка с выбором цвета и окрашиваемой иконкой
 */
class ColoredToolButton : public QToolButton
{
    Q_OBJECT

public:
    /**
     * @brief Виды цветовых панелей
     */
    enum ColorsPaneType {
        NonePane,
        Google,
        WordHighlight
    };

public:
    ColoredToolButton(const QIcon& _icon, QWidget* _parent = nullptr, QWidget* _topLevelParent = nullptr);
    ColoredToolButton(QWidget* _parent = nullptr, QWidget* _topLevelParent = nullptr);
    ~ColoredToolButton();

    /**
     * @brief Установить цветовую панель
     */
    void setColorsPane(ColorsPaneType _pane);

    /**
     * @brief Текущий цвет
     */
    QColor currentColor() const;

public slots:
    /**
     * @brief Установить цвет
     * @note Если устанавливается невалидный цвет, панель переходит в режим "цвет не выбран"
     */
    void setColor(const QColor& _color);

    /**
     * @brief Установить цвет без вызова сигнала clicked
     */
    void updateColor(const QColor& _color);

signals:
    /**
     * @brief Кнопка нажата
     */
    void clicked(const QColor& _color);

    /**
     * @brief Изменился цвет
     */
    void colorChanged(const QColor& _color);

protected:
    /**
     * @brief Переопределяем для обновления цвета иконки, при смене палитры
     */
    bool event(QEvent* _event);

private slots:
    /**
     * @brief Раскрасить иконку
     */
    void aboutUpdateIcon(const QColor& _color);

    /**
     * @brief Обработка нажатия на кнопку
     */
    void aboutClicked();

private:
    /**
     * @brief Иконка
     */
    QIcon m_icon;

    /**
     * @brief Индиктор того, что никакой цвет ещё не выбран
     */
    bool m_colorNotChoosedYet;

    /**
     * @brief Цветовая палитра
     */
    ColorsPane* m_colorsPane = nullptr;

#ifdef MOBILE_OS
    /**
     * @brief Выезжающая панель палитры
     */
    SlidingPanel* m_colorsPanel = nullptr;
#endif
};

#endif // COLOREDTOOLBUTTON_H
