#ifndef SCRIPTVERSIONWIDGET_H
#define SCRIPTVERSIONWIDGET_H

#include <QFrame>

namespace Ui {
    class ScriptVersionWidget;
}

namespace UserInterface
{
    /**
     * @brief Виджет для отображении версии сценария в списке
     */
    class ScriptVersionWidget : public QFrame
    {
        Q_OBJECT

    public:
        explicit ScriptVersionWidget(bool _isFirstVersion, QWidget* parent = nullptr);
        ~ScriptVersionWidget();

        /**
         * @brief Задать цвет версии
         */
        void setColor(const QColor& _color);

        /**
         * @brief Задать заголовок версии
         */
        void setTitle(const QString& _title);

        /**
         * @brief Задать описание версии
         */
        void setDescription(const QString& _description);

    signals:
        /**
         * @brief Нажат проект
         */
        void clicked();

        /**
         * @brief Нажата кнопка удалить
         */
        void removeClicked();

    protected:
#ifndef MOBILE_OS
        /**
         * @brief Переопределяем, чтобы изменять внешний вид виджета, в моменты входа/выхода
         *		  курсора мышки в границы виджета
         */
        /** @{ */
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        /** @} */

        /**
         * @brief Переопределяем для реализации события клика на версии
         */
        void mouseReleaseEvent(QMouseEvent* _event) override;
#endif

    private:
        /**
         * @brief Настроить представление
         */
        void initView();

        /**
         * @brief Настроить соединения
         */
        void initConnections();

        /**
         * @brief Настроить стиль
         */
        void initStylesheet();

        /**
         * @brief Установить флаг обозначающий находится ли мышка над элементом
         */
        void setMouseHover(bool _hover);

    private:
        /**
         * @brief Интерфейс
         */
        Ui::ScriptVersionWidget* m_ui = nullptr;

        /**
         * @brief Является ли версия первой
         */
        const bool m_isFirstVersion;
    };
}

#endif // SCRIPTVERSIONWIDGET_H
