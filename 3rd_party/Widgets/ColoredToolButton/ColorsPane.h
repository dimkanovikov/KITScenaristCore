#ifndef COLORSPANE_H
#define COLORSPANE_H

#include <QWidget>


/**
 * @brief Вспомогательная структура, для хранения связи цвета и его прямоугольника в палитре
 */
struct ColorKeyInfo
{
    QColor color;
    QRectF rect;
    bool isEnabled = true;

    ColorKeyInfo() {}
    ColorKeyInfo(const QColor& _color, const QRectF& _rect) : color(_color), rect(_rect) {}

    bool isValid() const {
        return color.isValid() && rect.isValid();
    }
};

/**
 * @brief Интерфейс палитры цветов
 */
class ColorsPane : public QWidget
{
    Q_OBJECT

public:
    explicit ColorsPane(QWidget* _parent = nullptr);

    /**
     * @brief Отключить возможность выбора заданного цвета
     */
    virtual void disableColor(const QColor& _color);

    /**
     * @brief Выбрать первый доступный к выбору цвет
     */
    virtual void selectFirstEnabledColor() = 0;

    /**
     * @brief Получить текущий цвет
     */
    QColor currentColor() const;

    /**
     * @brief Содержит ли панель заданный цвет
     */
    bool contains(const QColor& _color) const;

    /**
     * @brief Установить текущий цвет
     */
    void setCurrentColor(const QColor& _color);

signals:
    /**
    * @brief Выбран цвет
    */
    void selected(const QColor& _color);

protected:
    /**
     * @brief Настроить цвета
     */
    virtual void initColors() = 0;

    /**
     * @brief Получить цвета палитры
     */
    QList<ColorKeyInfo>& colorsInfos();

    /**
     * @brief Текущий цвет
     */
    ColorKeyInfo& currentColorInfo();

private:
    /**
     * @brief Все цвета палитры
     */
    QList<ColorKeyInfo> m_colorsInfos;

    /**
     * @brief Выбранный цвет палитры
     */
    ColorKeyInfo m_currentColorInfo;
};

#endif // COLORSPANE_H
