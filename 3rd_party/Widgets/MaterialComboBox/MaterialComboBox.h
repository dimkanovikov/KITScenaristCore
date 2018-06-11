#ifndef MATERIALCOMBOBOX_H
#define MATERIALCOMBOBOX_H

#include <QComboBox>

class QLabel;
class QStyledItemDelegate;


/**
 * @brief Выпадающий список в MaterialDesign стиле
 */
class MaterialComboBox : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialComboBox(QWidget* _parent = nullptr);

    /**
     * @brief Установить заголовок
     */
    void setLabel(const QString& _text);

    /**
     * @brief Установить модель с данными для списка
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Установить список строк в качестве модели
     */
    void setModel(const QStringList& _model);

    /**
     * @brief Установить текущий элемент по индексу
     */
    void setCurrentIndex(int _index);

    /**
     * @brief Установить текущий элемент списка
     */
    void setCurrentText(const QString& _text);

    /**
     * @brief Получить текст выбранного элемент
     */
    QString currentText() const;

signals:
    /**
     * @brief Изменился текущий элемент списка
     */
    void currentTextChanged(const QString& _text);

private:
    /**
     * @brief Заголовок
     */
    QLabel* m_label = nullptr;

    /**
     * @brief Выпадающий список
     */
    QComboBox* m_comboBox = nullptr;

    /**
     * @brief Делегат для отрисовки элементов выпадающего списка
     */
    QStyledItemDelegate* m_delegate = nullptr;
};

#endif // MATERIALCOMBOBOX_H
