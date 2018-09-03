#ifndef WORDHIGHLIGHTCOLORSPANE_H
#define WORDHIGHLIGHTCOLORSPANE_H


#include "ColorsPane.h"


/**
 * @brief Палитра цветов
 */
class WordHighlightColorsPane : public ColorsPane
{
	Q_OBJECT

public:
    explicit WordHighlightColorsPane(QWidget* _parent = nullptr);

    /**
     * @brief Выбрать первый доступный к выбору цвет
     */
    void selectFirstEnabledColor() override;

protected:
	/**
	 * @brief Переопределяются, для реализации логики работы палитры
	 */
	/** @{ */
    void paintEvent(QPaintEvent* _event) override;
    void mouseMoveEvent(QMouseEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
	/** @} */

	/**
	 * @brief Инициилизировать цвета панели
	 */
    void initColors() override;
};

#endif // WORDHIGHLIGHTCOLORSPANE_H
