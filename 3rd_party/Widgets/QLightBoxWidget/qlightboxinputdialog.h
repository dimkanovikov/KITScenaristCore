#ifndef QLIGHTBOXINPUTDIALOG_H
#define QLIGHTBOXINPUTDIALOG_H

#include "qlightboxdialog.h"

class QAbstractItemModel;
class QDialogButtonBox;
class QLabel;
#ifndef MOBILE_OS
class QLineEdit;
#else
class MaterialLineEdit;
#endif
class QListWidget;
class SimpleTextEditorWidget;


/**
 * @brief Фабрика для простых диалогов ввода
 */
class QLightBoxInputDialog : public QLightBoxDialog
{
    Q_OBJECT

public:
    /**
     * @brief Получить текст
     */
    static QString getText(QWidget* _parent, const QString& _title, const QString& _label,
        const QString& _text = QString());

    /**
     * @brief Получить большой текст
     */
    static QString getLongText(QWidget* _parent, const QString& _title, const QString& _label,
        const QString& _text = QString());

    /**
     * @brief Выбор элемента из списка
     */
    /** @{ */
    static QString getItem(QWidget* _parent, const QString& _title, const QStringList& _items,
        const QString& _selectedItem = QString());
    static QString getItem(QWidget* _parent, const QString& _title, const QAbstractItemModel* _itemsModel,
        const QString& _selectedItem = QString());
    /** @} */

private:
    explicit QLightBoxInputDialog(QWidget* _parent = 0, bool _isContentStretchable = false);

    /**
     * @brief Настроить представление
     */
    void initView() override;

    /**
     * @brief Настроить соединения
     */
    void initConnections() override;

    /**
     * @brief Виджет на который установить фокус при отображении
     */
    QWidget* focusedOnExec() const override;

private:
    /**
     * @brief Текстовая метка
     */
    QLabel* m_label = nullptr;

    /**
     * @brief Поле для текстового ввода
     */
#ifndef MOBILE_OS
    QLineEdit* m_lineEdit = nullptr;
#else
    MaterialLineEdit* m_lineEdit = nullptr;
#endif

    /**
     * @brief Поле для ввода большого кол-ва текста
     */
    SimpleTextEditorWidget* m_textEdit = nullptr;

    /**
     * @brief Виджет для обработки списковых операций
     */
    QListWidget* m_listWidget = nullptr;

    /**
     * @brief Кнопки диалога
     */
    QDialogButtonBox* m_buttons = nullptr;
};

#endif // QLIGHTBOXINPUTDIALOG_H
