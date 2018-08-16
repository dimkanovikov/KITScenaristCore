#include "ScriptVersions.h"
#include "ScriptVersionWidget.h"

#include <Domain/ScriptVersion.h>

#include <3rd_party/Helpers/TextUtils.h>

#include <QAbstractItemModel>
#include <QVBoxLayout>

using Domain::ScriptVersionsTable;
using UserInterface::ScriptVersions;
using UserInterface::ScriptVersionWidget;


ScriptVersions::ScriptVersions(QWidget* _parent)
    : QScrollArea(_parent)
{
    initView();
}

void ScriptVersions::setModel(QAbstractItemModel* _model)
{
    QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>(widget()->layout());

    //
    // Стираем старый список версий
    //
    while (layout->count() > 0) {
        QLayoutItem* item = layout->takeAt(0);
        if (item != nullptr
            && item->widget() != nullptr) {
            item->widget()->deleteLater();
        }
    }

    //
    // Отключаем старую модель
    //
    if (m_model != nullptr) {
        m_model->disconnect(this);
    }

    //
    // Сохраняем новую модель
    //
    m_model = _model;

    //
    // Строим новый список версий
    //
    if (m_model != nullptr) {
        for (int row = m_model->rowCount() - 1; row >= 0; --row) {
            ScriptVersionWidget* version = new ScriptVersionWidget;
            const QString versionName = m_model->index(row, ScriptVersionsTable::kName).data().toString();
            const QString versionDateTime = m_model->index(row, ScriptVersionsTable::kDatetime).data().toDateTime().toString("dd.MM.yyyy hh:mm:ss");
            version->setTitle(QString("%1 %2").arg(versionName).arg(TextUtils::directedText(versionDateTime, '[', ']')));
            const QString versionDescription = m_model->index(row, ScriptVersionsTable::kDescription).data().toString();
            version->setDescription(versionDescription);
            const QColor versionColor = m_model->index(row, ScriptVersionsTable::kColor).data().value<QColor>();
            version->setColor(versionColor);
            //
            connect(version, &ScriptVersionWidget::removeClicked, this, &ScriptVersions::handleRemoveClick);

            layout->addWidget(version);
        }

        layout->addStretch(1);
    }

    //
    // FIXME: сделать нормальное управление изменениями модели
    //
    connect(m_model, &QAbstractItemModel::rowsInserted, this, [this] { setModel(m_model); });
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, [this] { setModel(m_model); });
}

void ScriptVersions::initView()
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    QWidget* content = new QWidget;
    content->setObjectName("ProjectListsContent");
    content->setLayout(layout);

    setWidget(content);
    setWidgetResizable(true);
}

int ScriptVersions::versionRow(ScriptVersionWidget* _version) const
{
    //
    // Инвертируем индекс, т.к. на экране мы отображаем от новых к старым
    //
    QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>(widget()->layout());
    return layout->count() - layout->indexOf(_version) - 2; // Отнимаем два т.к. индексы с 0 + одна позиция на спейсер
}

void ScriptVersions::handleRemoveClick()
{
    if (ScriptVersionWidget* version = qobject_cast<ScriptVersionWidget*>(sender())) {
        emit removeRequested(m_model->index(versionRow(version), 0));
    }
}
