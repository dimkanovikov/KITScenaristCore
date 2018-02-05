#include "MaterialComboBox.h"

#include <QAbstractItemView>
#include <QLabel>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QVBoxLayout>


MaterialComboBox::MaterialComboBox(QWidget* _parent) :
    QWidget(_parent),
    m_label(new QLabel(this)),
    m_comboBox(new QComboBox(this)),
    m_delegate(new QStyledItemDelegate(this))
{
    setFocusProxy(m_comboBox);
    m_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    layout->addWidget(m_label);
    layout->addWidget(m_comboBox);
    setLayout(layout);

    connect(m_comboBox, &QComboBox::currentTextChanged, this, &MaterialComboBox::currentTextChanged);

    m_label->setProperty("materialComboBoxLabel", true);
    m_comboBox->setProperty("materialComboBox", true);
}

void MaterialComboBox::setLabel(const QString& _text)
{
    m_label->setText(_text);
}

void MaterialComboBox::setModel(QAbstractItemModel* _model)
{
    m_comboBox->setModel(_model);
    m_comboBox->view()->setItemDelegate(m_delegate);
}

void MaterialComboBox::setModel(const QStringList& _model)
{
    setModel(new QStringListModel(_model, m_comboBox));
}

void MaterialComboBox::setCurrentText(const QString& _text)
{
    m_comboBox->setCurrentText(_text);
}

QString MaterialComboBox::currentText() const
{
    return m_comboBox->currentText();
}
