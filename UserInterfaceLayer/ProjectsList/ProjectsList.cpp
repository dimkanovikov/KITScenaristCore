#include "ProjectsList.h"

#include "ProjectFileWidget.h"

#include <QAbstractItemModel>
#include <QDateTime>
#include <QVBoxLayout>

using UserInterface::ProjectsList;
using UserInterface::ProjectFileWidget;


ProjectsList::ProjectsList(QWidget* _parent) :
    QScrollArea(_parent)
{
    initView();
}

void ProjectsList::setModel(QAbstractItemModel* _model, bool _isRemote)
{
    QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>(widget()->layout());

    //
    // Стираем старый список проектов
    //
    while (layout->count() > 0) {
        QLayoutItem* item = layout->takeAt(0);
        if (item != nullptr
            && item->widget() != nullptr) {
            item->widget()->deleteLater();
        }
    }

    //
    // Сохраняем новую модель
    //
    m_model = _model;

    //
    // Строим новый список проектов
    //
    if (m_model != nullptr) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            ProjectFileWidget* project = new ProjectFileWidget;
            const QModelIndex projectIndex = m_model->index(row, 0);
            const QString projectName = projectIndex.data(Qt::DisplayRole).toString();
            project->setProjectName(projectName);
#ifdef MOBILE_OS
            const QDateTime editDateTime = projectIndex.data(Qt::WhatsThisRole).toDateTime();
            project->setProjectInfo(editDateTime.isNull()
                                    ? tr("no changes")
                                    : editDateTime.toString("dd.MM.yyyy hh:mm:ss"));
#else
            const QString projectPath = projectIndex.data(Qt::StatusTipRole).toString();
            project->setProjectInfo(projectPath);
#endif
            const bool isOwner = projectIndex.data(Qt::UserRole + 1).toBool();
            project->configureOptions(_isRemote, isOwner);
            const QStringList users = projectIndex.data(Qt::UserRole).toStringList();
            for (const QString& user : users) {
                if (!user.simplified().isEmpty()) {
                    const QStringList userInfo = user.split(";");
                    project->addCollaborator(userInfo.value(0), userInfo.value(1), userInfo.value(2), isOwner);
                }
            }
            //
            connect(project, &ProjectFileWidget::clicked, this, &ProjectsList::handleProjectClick);
            connect(project, &ProjectFileWidget::editClicked, this, &ProjectsList::handleEditClick);
            connect(project, &ProjectFileWidget::removeClicked, this, &ProjectsList::handleRemoveClick);
            connect(project, &ProjectFileWidget::hideClicked, this, &ProjectsList::handleHideClick);
            connect(project, &ProjectFileWidget::moveToCloudClicked, this, &ProjectsList::handleMoveToCloudClick);
            connect(project, &ProjectFileWidget::shareClicked, this, &ProjectsList::handleShareClick);
            connect(project, &ProjectFileWidget::removeUserRequested, this, &ProjectsList::handleRemoveUserRequest);

            layout->addWidget(project);
        }

        layout->addStretch(1);
    }
}

void ProjectsList::setProjectName(int _index, const QString& _name)
{
    QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>(widget()->layout());
    if (layout->count() > _index) {
        QWidget* projectWidget = layout->itemAt(_index)->widget();
        if (ProjectFileWidget* project = qobject_cast<ProjectFileWidget*>(projectWidget)) {
            project->setProjectName(_name);
        }
    }
}

void ProjectsList::setMenusVisible(bool _isVisible)
{
    QLayout* layout = widget()->layout();
    for (int i = 0; i != layout->count(); ++i) {
        QWidget* widget = layout->itemAt(i)->widget();
        ProjectFileWidget* projectFileWidget = qobject_cast<ProjectFileWidget*>(widget);
        if (projectFileWidget) {
            projectFileWidget->setMenuVisible(_isVisible);
        }
    }
}

bool ProjectsList::hasProjects() const
{
    return m_model != nullptr
           && m_model->rowCount() > 0;
}

int ProjectsList::projectRow(UserInterface::ProjectFileWidget* _project) const
{
    QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>(widget()->layout());
    return layout->indexOf(_project);
}

void ProjectsList::handleProjectClick()
{
    if (ProjectFileWidget* project = qobject_cast<ProjectFileWidget*>(sender())) {
        emit clicked(m_model->index(projectRow(project), 0));
    }
}

void ProjectsList::handleEditClick()
{
    if (ProjectFileWidget* project = qobject_cast<ProjectFileWidget*>(sender())) {
        emit editRequested(m_model->index(projectRow(project), 0));
    }
}

void ProjectsList::handleRemoveClick()
{
    if (ProjectFileWidget* project = qobject_cast<ProjectFileWidget*>(sender())) {
        emit removeRequested(m_model->index(projectRow(project), 0));
    }
}

void ProjectsList::handleHideClick()
{
    if (ProjectFileWidget* project = qobject_cast<ProjectFileWidget*>(sender())) {
        emit hideRequested(m_model->index(projectRow(project), 0));
    }
}

void ProjectsList::handleMoveToCloudClick()
{
    if (ProjectFileWidget* project = qobject_cast<ProjectFileWidget*>(sender())) {
        emit moveToCloudRequested(m_model->index(projectRow(project), 0));
    }
}

void ProjectsList::handleShareClick()
{
    if (ProjectFileWidget* project = qobject_cast<ProjectFileWidget*>(sender())) {
        emit shareRequested(m_model->index(projectRow(project), 0));
    }
}

void ProjectsList::handleRemoveUserRequest(const QString& _email)
{
    if (ProjectFileWidget* project = qobject_cast<ProjectFileWidget*>(sender())) {
        emit unshareRequested(m_model->index(projectRow(project), 0), _email);
    }
}

void ProjectsList::initView()
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
