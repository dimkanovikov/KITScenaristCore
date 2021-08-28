#include "SettingsStorage.h"

#include <DataLayer/DataMappingLayer/MapperFacade.h>
#include <DataLayer/DataMappingLayer/SettingsMapper.h>

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

#include <3rd_party/Helpers/ShortcutHelper.h>

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QHeaderView>
#include <QRadioButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QStringList>
#include <QTableView>
#include <QToolButton>
#include <QTreeView>
#include <QUuid>
#include <QWidget>

using namespace DataStorageLayer;
using namespace DataMappingLayer;
using namespace BusinessLogic;

namespace {
    /**
     * @brief Ключ для хранения положения и размеров интерфейса
     */
    const QString STATE_AND_GEOMETRY_KEY = "state_and_geometry";

    /**
     * @brief Имя пользователя из системы
     */
    static QString systemUserName() {
        QString name = qgetenv("USER");
        if (name.isEmpty()) {
            //
            // Windows
            //
            name = QString::fromLocal8Bit(qgetenv("USERNAME"));
            if (name.isEmpty()) {
                name = "user";
            }
        }
        return name;
    }

    /**
     * @brief Преобразовать список чисел в строку
     */
    static QString intListToString(const QList<int>& _values) {
        QString result;
        for (const int& value : _values) {
            result.append(QString("%1#").arg(value));
        }
        return result;
    }

    /**
     * @brief Преобразовать строку в список чисел
     */
    static QList<int> intListFromString(const QString& _values) {
        QList<int> result;
        for (const QString& value : _values.split("#", QString::SkipEmptyParts)) {
            result.append(value.toInt());
        }
        return result;
    }
}


void SettingsStorage::setVariantValue(const QString& _key, const QVariant& _value, SettingsStorage::SettingsPlace _settingsPlace)
{
    //
    // Кэшируем значение
    //
    cacheValue(_key, _value, _settingsPlace);

    //
    // Сохраняем его в заданное хранилище
    //
    if (_settingsPlace == ApplicationSettings) {
        m_appSettings.setValue(_key.toUtf8().toHex(), _value);
    } else {
        MapperFacade::settingsMapper()->setValue(_key, _value.toString());
    }
}

void SettingsStorage::setValue(const QString& _key, const QString& _value, SettingsPlace _settingsPlace)
{
    setVariantValue(_key, _value, _settingsPlace);
}

void SettingsStorage::setValue(const QString& _key, const QStringList& _value, SettingsStorage::SettingsPlace _settingsPlace)
{
    setVariantValue(_key, _value, _settingsPlace);
}

void SettingsStorage::setValues(const QMap<QString, QString>& _values, const QString& _valuesGroup, SettingsStorage::SettingsPlace _settingsPlace)
{
    //
    // Кэшируем значение
    //
    cacheValue(_valuesGroup, QVariant::fromValue<QMap<QString, QString> >(_values), _settingsPlace);

    //
    // Сохраняем его в заданное хранилище
    //
    if (_settingsPlace == ApplicationSettings) {
        //
        // Очистим группу
        //
        {
            m_appSettings.beginGroup(_valuesGroup);
            m_appSettings.remove("");
            m_appSettings.endGroup();
        }

        //
        // Откроем группу
        //
        m_appSettings.beginGroup(_valuesGroup);

        //
        // Сохраним значения
        //
        foreach (const QString& key, _values.keys()) {
            m_appSettings.setValue(key.toUtf8().toHex(), _values.value(key));
            m_cachedValuesApp.insert(key, _values.value(key));
        }

        //
        // Закроем группу
        //
        m_appSettings.endGroup();

        //
        // Сохраняем изменения в файл
        //
        m_appSettings.sync();
    }
    //
    // В базу данных карта параметров не умеет сохраняться
    //
    else {
        Q_ASSERT_X(0, Q_FUNC_INFO, "Database settings can't save group of settings");
    }

}

QVariant SettingsStorage::variantValue(const QString& _key, SettingsStorage::SettingsPlace _settingsPlace)
{
    //
    // Пробуем получить значение из кэша
    //
    bool hasCachedValue = false;
    QVariant value = getCachedValue(_key, _settingsPlace, hasCachedValue);

    //
    // Если в кэше нет, то загружаем из указанного места
    //
    if (!hasCachedValue) {
        if (_settingsPlace == ApplicationSettings) {
            value = m_appSettings.value(_key.toUtf8().toHex(), QVariant());
        } else {
            value = MapperFacade::settingsMapper()->value(_key);
        }

        //
        // Сохраняем значение в кэш
        //
        cacheValue(_key, value, _settingsPlace);
    }

    return value;
}

QString SettingsStorage::value(const QString& _key, SettingsPlace _settingsPlace, const QString& _defaultValue)
{
    //
    // Пробуем получить значение из кэша
    //
    bool hasCachedValue = false;
    QString value = getCachedValue(_key, _settingsPlace, hasCachedValue).toString();

    //
    // Если в кэше нет, то загружаем из указанного места
    //
    if (!hasCachedValue) {
        if (_settingsPlace == ApplicationSettings) {
            value = m_appSettings.value(_key.toUtf8().toHex(), QVariant()).toString();
        } else {
            value = MapperFacade::settingsMapper()->value(_key);
        }

        //
        // Если параметр не задан, то используем значение по умолчанию
        //
        if (value.isEmpty()) {
            if (_defaultValue.isEmpty()) {
                value = m_defaultValues.value(_key);
            } else {
                value = _defaultValue;
            }
        }

        //
        // Сохраняем значение в кэш
        //
        cacheValue(_key, value, _settingsPlace);
    }

    return value;
}

QMap<QString, QString> SettingsStorage::values(const QString& _valuesGroup, SettingsStorage::SettingsPlace _settingsPlace)
{
    //
    // Пробуем получить значение из кэша
    //
    bool hasCachedValue = false;
    QMap<QString, QString> settingsValues =
        getCachedValue(_valuesGroup, _settingsPlace, hasCachedValue).value<QMap<QString, QString> >();

    //
    // Если в кэше нет, то загружаем из указанного места
    //
    if (!hasCachedValue) {
        if (_settingsPlace == ApplicationSettings) {
            //
            // Откроем группу для считывания
            //
            m_appSettings.beginGroup(_valuesGroup);

            //
            // Получим все ключи
            //
            QStringList keys = m_appSettings.childKeys();

            //
            // Получим все значения
            //
            foreach (QString key, keys) {
                 settingsValues.insert(QByteArray::fromHex(key.toUtf8()), m_appSettings.value(key).toString());
            }

            //
            // Закроем группу
            //
            m_appSettings.endGroup();
        }
        //
        // Из базы данных карта параметров не умеет загружаться
        //
        else {
            Q_ASSERT_X(0, Q_FUNC_INFO, "Database settings can't load group of settings");
        }

        //
        // Сохраняем значение в кэш
        //
        cacheValue(_valuesGroup, QVariant::fromValue<QMap<QString, QString> >(settingsValues), _settingsPlace);
    }

    return settingsValues;
}

void SettingsStorage::resetValues(SettingsStorage::SettingsPlace _settingsPlace)
{
    if (_settingsPlace == ApplicationSettings) {
        //
        // Сбрасываем кэш
        //
        m_cachedValuesApp.clear();

        //
        // Восстанавливаем значения по умолчанию
        //
        foreach (const QString& key, m_defaultValues.keys()) {
            setValue(key, m_defaultValues.value(key), _settingsPlace);
            QApplication::processEvents();
        }
    }
    else {
        Q_ASSERT_X(0, Q_FUNC_INFO, "Can't reset settings stored in database.");
    }
}

void SettingsStorage::saveApplicationStateAndGeometry(QWidget* _widget)
{
    m_appSettings.beginGroup(STATE_AND_GEOMETRY_KEY);

    //
    // Геометрия и положение самого окна
    //
    m_appSettings.setValue("geometry", _widget->saveGeometry());

    //
    // Сохраняем состояния кнопок
    //
    m_appSettings.beginGroup("toolbuttons");
    foreach (QToolButton* toolButton, _widget->findChildren<QToolButton*>()) {
        m_appSettings.setValue(toolButton->objectName() + "-checked", toolButton->isChecked());
    }
    m_appSettings.endGroup();

    //
    // Сохраняем состояния разделителей
    //
    m_appSettings.beginGroup("splitters");
    foreach (QSplitter* splitter, _widget->findChildren<QSplitter*>()) {
        m_appSettings.beginGroup(splitter->objectName());
        m_appSettings.setValue("state", splitter->saveState());
        m_appSettings.setValue("geometry", splitter->saveGeometry());
        //
        // Если разделитель был инициилизирован, сохраним его размеры
        //
        {
            bool isSplitterInitialized = std::max(splitter->width(), splitter->height()) > 200;
            bool isSplitterSizesLoaded = false;
            for (int size : splitter->sizes()) {
                if (size > 0) {
                    isSplitterSizesLoaded = true;
                    break;
                }
            }
            if (isSplitterInitialized && isSplitterSizesLoaded) {
                m_appSettings.setValue("sizes", ::intListToString(splitter->sizes()));
            }
        }
        //
        // Сохраняем расположение панелей
        //
        m_appSettings.beginGroup("splitter-widgets");
        for (int widgetPos = 0; widgetPos < splitter->count(); ++widgetPos) {
            m_appSettings.setValue(splitter->widget(widgetPos)->objectName(), widgetPos);
        }
        m_appSettings.endGroup(); // splitter-widgets
        m_appSettings.endGroup(); // splitter->objectName()
    }
    m_appSettings.endGroup();

    //
    // Сохраняем состояния таблиц
    //
    m_appSettings.beginGroup("headers");
    foreach (QTableView* table, _widget->findChildren<QTableView*>()) {
        m_appSettings.setValue(table->objectName() + "-state", table->horizontalHeader()->saveState());
        m_appSettings.setValue(table->objectName() + "-geometry", table->horizontalHeader()->saveGeometry());
    }
    //
    // ... и деревьев
    //
    foreach (QTreeView* tree, QApplication::activeWindow()->findChildren<QTreeView*>()) {
        m_appSettings.setValue(tree->objectName() + "-state", tree->header()->saveState());
        m_appSettings.setValue(tree->objectName() + "-geometry", tree->header()->saveGeometry());
    }
    m_appSettings.endGroup();

    //
    // Сохраняем состояния радиокнопок
    //
    m_appSettings.beginGroup("radiobuttons");
    foreach (QRadioButton* radioButton, _widget->findChildren<QRadioButton*>()) {
        m_appSettings.setValue(radioButton->objectName() + "-checked", radioButton->isChecked());
    }
    m_appSettings.endGroup();

    //
    // Сохраняем состояния чекбоксов
    //
    m_appSettings.beginGroup("checkboxes");
    foreach (QCheckBox* checkBox, _widget->findChildren<QCheckBox*>()) {
        m_appSettings.setValue(checkBox->objectName() + "-checked", checkBox->isChecked());
    }
    m_appSettings.endGroup();

    //
    // Сохраняем состояния слайдеров
    //
    m_appSettings.beginGroup("sliders");
    foreach (QAbstractSlider* slider, _widget->findChildren<QAbstractSlider*>()) {
        m_appSettings.setValue(slider->objectName() + "-value", slider->value());
    }
    m_appSettings.endGroup();

    //
    // Сохраняем состояния спинбоксов
    //
    m_appSettings.beginGroup("spinboxes");
    foreach (QSpinBox* spinBox, _widget->findChildren<QSpinBox*>()) {
        m_appSettings.setValue(spinBox->objectName() + "-value", spinBox->value());
    }
    m_appSettings.endGroup();

    m_appSettings.endGroup(); // STATE_AND_GEOMETRY_KEY
}

void SettingsStorage::loadApplicationStateAndGeometry(QWidget* _widget)
{
    m_appSettings.beginGroup(STATE_AND_GEOMETRY_KEY);

    //
    // Восстановим геометрию и положение окна
    //
    _widget->restoreGeometry(m_appSettings.value("geometry").toByteArray());

    //
    // Состояния кнопок
    //
    m_appSettings.beginGroup("toolbuttons");
    foreach (QToolButton* toolButton, _widget->findChildren<QToolButton*>()) {
        toolButton->setChecked(m_appSettings.value(toolButton->objectName() + "-checked", false).toBool());
    }
    m_appSettings.endGroup();

    //
    // Разделителей
    //
    m_appSettings.beginGroup("splitters");
    foreach (QSplitter* splitter, _widget->findChildren<QSplitter*>()) {
        m_appSettings.beginGroup(splitter->objectName());
        //
        // Восстанавливаем очерёдность расположения панелей
        //
        m_appSettings.beginGroup("splitter-widgets");
        //
        // ... сформируем карту позиционирования
        //
        QMap<int, QWidget*> splitterWidgets;
        for (int widgetPos = 0; widgetPos < splitter->count(); ++widgetPos) {
            QWidget* widget = splitter->widget(widgetPos);
            if (!m_appSettings.value(widget->objectName()).isNull()) {
                const int position = m_appSettings.value(widget->objectName()).toInt();
                splitterWidgets.insert(position, widget);
            }
        }
        //
        // ... позиционируем сами виджеты
        //
        foreach (int position, splitterWidgets.keys()) {
            splitter->insertWidget(position, splitterWidgets.value(position));
        }
        m_appSettings.endGroup(); // splitter-widgets

        //
        // Восстанавливаем состояние и геометрию разделителя
        //
        // Это необходимо делать только после восстановления расположения панелей, т.к. некоторые
        // панели имеют определённый минимальный размер, соответственно этот размер задаётся и
        // области разделителя, в которой он находится
        //
        splitter->restoreState(m_appSettings.value("state").toByteArray());
        splitter->restoreGeometry(m_appSettings.value("geometry").toByteArray());
        //
        // Запрещаем схлапывание вертикальных разделителей
        //
        splitter->setChildrenCollapsible(splitter->orientation() == Qt::Horizontal);
        //
        // Восстанавливаем размеры
        //
        if (m_appSettings.contains("sizes")
            && !splitter->childrenCollapsible()) {
            const QList<int> sizes = ::intListFromString(m_appSettings.value("sizes").toString());
            //
            // Если у сплиттера нельзя схлапывать панели вручную, но размер должен быть нулевым,
            // то сперва скрываем необходимые виджеты, а уже потом восстанавливаем состояние
            //
            for (int index = 0; index < sizes.size(); ++index) {
                if (sizes.at(index) == 0) {
                    splitter->widget(index)->hide();
                }
            }
            splitter->setSizes(sizes);
        }

        m_appSettings.endGroup(); // splitter->objectName()
    }
    m_appSettings.endGroup();

    //
    // Таблиц
    //
    m_appSettings.beginGroup("headers");
    foreach (QTableView* table, _widget->findChildren<QTableView*>()) {
        table->horizontalHeader()->restoreState(m_appSettings.value(table->objectName() + "-state").toByteArray());
        table->horizontalHeader()->restoreGeometry(m_appSettings.value(table->objectName() + "-geometry").toByteArray());
    }
    //
    // ... и деревьев
    //
    foreach (QTreeView* tree, _widget->findChildren<QTreeView*>()) {
        tree->header()->restoreState(m_appSettings.value(tree->objectName() + "-state").toByteArray());
        tree->header()->restoreGeometry(m_appSettings.value(tree->objectName() + "-geometry").toByteArray());
    }
    m_appSettings.endGroup();


    //
    // Радиокнопок
    //
    m_appSettings.beginGroup("radiobuttons");
    foreach (QRadioButton* radioButton, _widget->findChildren<QRadioButton*>()) {
        //
        // Игнорируем состояние радиокнопок со списками проектов, там всегда должно загружаться предустановленное значение
        //
        if (radioButton->objectName() != "localProjects"
            && radioButton->objectName() != "remoteProjects") {
            radioButton->setChecked(m_appSettings.value(radioButton->objectName() + "-checked", radioButton->isChecked()).toBool());
        }
    }
    m_appSettings.endGroup();

    //
    // Чекбоксов
    //
    m_appSettings.beginGroup("checkboxes");
    foreach (QCheckBox* checkBox, _widget->findChildren<QCheckBox*>()) {
        checkBox->setChecked(m_appSettings.value(checkBox->objectName() + "-checked", checkBox->isChecked()).toBool());
    }
    m_appSettings.endGroup();

    //
    // Слайдеров
    //
    m_appSettings.beginGroup("sliders");
    foreach (QAbstractSlider* slider, _widget->findChildren<QAbstractSlider*>()) {
        slider->setValue(m_appSettings.value(slider->objectName() + "-value", slider->value()).toInt());
    }
    m_appSettings.endGroup();

    //
    // Спинбоксов
    //
    m_appSettings.beginGroup("spinboxes");
    foreach (QSpinBox* spinBox, _widget->findChildren<QSpinBox*>()) {
        spinBox->setValue(m_appSettings.value(spinBox->objectName() + "-value", spinBox->value()).toInt());
    }
    m_appSettings.endGroup();

    m_appSettings.endGroup(); // STATE_AND_GEOMETRY_KEY
}

void SettingsStorage::resetApplicationStateAndGeometry()
{
    m_appSettings.remove(STATE_AND_GEOMETRY_KEY);
}

QString SettingsStorage::documentFolderPath(const QString& _key)
{
    QString folderPath = value(_key, ApplicationSettings);
    if (folderPath.isEmpty()) {
        folderPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    return folderPath;
}

QString SettingsStorage::documentFilePath(const QString& _key, const QString& _fileName)
{
    QString filePath = documentFolderPath(_key) + QDir::separator() + _fileName;
    return QDir::toNativeSeparators(filePath);
}

void SettingsStorage::saveDocumentFolderPath(const QString& _key, const QString& _filePath)
{
    setValue(_key, QFileInfo(_filePath).absoluteDir().absolutePath(), ApplicationSettings);
}

SettingsStorage::SettingsStorage()
{
    //
    // Настроим значения параметров по умолчанию
    //
    m_defaultValues.insert("application/current-app-review-version", "12");
    m_defaultValues.insert("application/uuid", QUuid::createUuid().toString());
    m_defaultValues.insert("application/app-was-configured", "0");
    m_defaultValues.insert("application/language", "-1");
    m_defaultValues.insert("application/username", ::systemUserName());
    m_defaultValues.insert("application/use-dark-theme", "1");
    m_defaultValues.insert("application/use-preedit",
#ifdef Q_OS_ANDROID
                           "0"
#else
                           "1"
#endif
                           );
    m_defaultValues.insert("application/autosave", "1");
    m_defaultValues.insert("application/autosave-interval", "5");
    m_defaultValues.insert("application/save-backups", "1");
    m_defaultValues.insert("application/save-backups-folder",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/KITScenarist/backups");
    m_defaultValues.insert("application/compact-mode-auto-enable", "1");
    m_defaultValues.insert("application/compact-mode", "0");
    m_defaultValues.insert("application/modules/research", "1");
    m_defaultValues.insert("application/modules/cards", "1");
    m_defaultValues.insert("application/modules/scenario", "1");
    m_defaultValues.insert("application/modules/statistics", "1");
    m_defaultValues.insert("application/modules/tools", "1");

    m_defaultValues.insert("research/default-font/family", "Courier Prime");
    m_defaultValues.insert("research/default-font/size", "12");


    m_defaultValues.insert("cards/use-corkboard", "1");
    m_defaultValues.insert("cards/background-color", "#FEFEFE");
    m_defaultValues.insert("cards/background-color-dark", "#26282A");

    m_defaultValues.insert("navigator/show-scenes-numbers", "1");
    m_defaultValues.insert("navigator/show-scene-description", "1");
    m_defaultValues.insert("navigator/scene-description-is-scene-text", "1");
    m_defaultValues.insert("navigator/scene-description-height", "1");

    m_defaultValues.insert("scenario-editor/current-style",
#ifndef MOBILE_OS
                          "default"
#else
                          "Mobile"
#endif
                           );
    m_defaultValues.insert("scenario-editor/page-view",
#ifndef MOBILE_OS
                           "1"
#else
                           "0"
#endif
                           );
    m_defaultValues.insert("scenario-editor/zoom-range", "1");
    m_defaultValues.insert("scenario-editor/show-scenes-numbers", "0");
    m_defaultValues.insert("scenario-editor/show-dialogues-numbers", "0");
    m_defaultValues.insert("scenario-editor/hide-panels-in-fullscreen", "1");
    m_defaultValues.insert("scenario-editor/highlight-blocks", "0");
    m_defaultValues.insert("scenario-editor/highlight-current-line", "0");
    m_defaultValues.insert("scenario-editor/text-color", "#000000");
    m_defaultValues.insert("scenario-editor/background-color", "#FEFEFE");
    m_defaultValues.insert("scenario-editor/nonprintable-text-color", "#0AC139");
    m_defaultValues.insert("scenario-editor/folder-text-color", "#FEFEFE");
    m_defaultValues.insert("scenario-editor/folder-background-color", "#CAC6C3");
    m_defaultValues.insert("scenario-editor/text-color-dark", "#EBEBEB");
    m_defaultValues.insert("scenario-editor/background-color-dark",
#ifndef MOBILE_OS
                           "#3D3D3D"
#else
                           "#26282A"
#endif
                           );
    m_defaultValues.insert("scenario-editor/nonprintable-text-color-dark", "#0AC139");
    m_defaultValues.insert("scenario-editor/folder-text-color-dark", "#EBEBEB");
    m_defaultValues.insert("scenario-editor/folder-background-color-dark", "#8D2DC4");
    m_defaultValues.insert("scenario-editor/auto-continue-dialogue", "0");
    m_defaultValues.insert("scenario-editor/auto-corrections-on-page-breaks", "1");
    m_defaultValues.insert("scenario-editor/capitalize-first-word", "1");
    m_defaultValues.insert("scenario-editor/correct-double-capitals", "1");
    m_defaultValues.insert("scenario-editor/replace-three-dots", "0");
    m_defaultValues.insert("scenario-editor/smart-quotes", "0");
    m_defaultValues.insert("scenario-editor/auto-styles-jumping", "1");
    m_defaultValues.insert("scenario-editor/show-suggestions-in-empty-blocks-2", "1");
    m_defaultValues.insert("scenario-editor/show-suggestions-in-empty-blocks", "1");
    m_defaultValues.insert("scenario-editor/spell-checking", "0");
    m_defaultValues.insert("scenario-editor/spell-checking-language", "2");
    m_defaultValues.insert("scenario-editor/zoom-range", "0");
    //
    m_defaultValues.insert("scenario-editor/shortcuts/scene_heading", ShortcutHelper::makeShortcut("Ctrl+Return"));
    m_defaultValues.insert("scenario-editor/shortcuts/scene_characters", ShortcutHelper::makeShortcut("Ctrl+E"));
    m_defaultValues.insert("scenario-editor/shortcuts/action", ShortcutHelper::makeShortcut("Ctrl+J"));
    m_defaultValues.insert("scenario-editor/shortcuts/character", ShortcutHelper::makeShortcut("Ctrl+U"));
    m_defaultValues.insert("scenario-editor/shortcuts/dialog", ShortcutHelper::makeShortcut("Ctrl+L"));
    m_defaultValues.insert("scenario-editor/shortcuts/parenthetical", ShortcutHelper::makeShortcut("Ctrl+H"));
    m_defaultValues.insert("scenario-editor/shortcuts/transition", ShortcutHelper::makeShortcut("Ctrl+G"));
    m_defaultValues.insert("scenario-editor/shortcuts/note", ShortcutHelper::makeShortcut("Ctrl+P"));
    m_defaultValues.insert("scenario-editor/shortcuts/title", ShortcutHelper::makeShortcut("Ctrl+N"));
    m_defaultValues.insert("scenario-editor/shortcuts/noprintable_text", ShortcutHelper::makeShortcut("Ctrl+Esc"));
    m_defaultValues.insert("scenario-editor/shortcuts/scene_group_header", ShortcutHelper::makeShortcut("Ctrl+D"));
    m_defaultValues.insert("scenario-editor/shortcuts/folder_header", ShortcutHelper::makeShortcut("Ctrl+Space"));
    m_defaultValues.insert("scenario-editor/shortcuts/lyrics", ShortcutHelper::makeShortcut("Ctrl+K"));
    //
    m_defaultValues.insert("scenario-editor/styles-jumping/from-scene_heading-by-tab", QString::number(ScenarioBlockStyle::SceneCharacters));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-scene_heading-by-enter", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-scene_characters-by-tab", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-scene_characters-by-enter", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-action-by-tab", QString::number(ScenarioBlockStyle::Character));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-action-by-enter", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-character-by-tab", QString::number(ScenarioBlockStyle::Parenthetical));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-character-by-enter", QString::number(ScenarioBlockStyle::Dialogue));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-dialog-by-tab", QString::number(ScenarioBlockStyle::Parenthetical));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-dialog-by-enter", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-parenthetical-by-tab", QString::number(ScenarioBlockStyle::Dialogue));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-parenthetical-by-enter", QString::number(ScenarioBlockStyle::Dialogue));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-transition-by-tab", QString::number(ScenarioBlockStyle::SceneHeading));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-transition-by-enter", QString::number(ScenarioBlockStyle::SceneHeading));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-note-by-tab", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-note-by-enter", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-title-by-tab", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-title-by-enter", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-noprintable_text-by-tab", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-noprintable_text-by-enter", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-scene_group_header-by-tab", QString::number(ScenarioBlockStyle::SceneHeading));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-scene_group_header-by-enter", QString::number(ScenarioBlockStyle::SceneHeading));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-folder_header-by-tab", QString::number(ScenarioBlockStyle::SceneHeading));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-folder_header-by-enter", QString::number(ScenarioBlockStyle::SceneHeading));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-lyrics-by-tab", QString::number(ScenarioBlockStyle::Parenthetical));
    m_defaultValues.insert("scenario-editor/styles-jumping/from-lyrics-by-enter", QString::number(ScenarioBlockStyle::Action));
    //
    m_defaultValues.insert("scenario-editor/styles-changing/from-scene_heading-by-tab", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-changing/from-scene_heading-by-enter", QString::number(ScenarioBlockStyle::SceneHeading));
    m_defaultValues.insert("scenario-editor/styles-changing/from-scene_characters-by-tab", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-changing/from-scene_characters-by-enter", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-changing/from-action-by-tab", QString::number(ScenarioBlockStyle::Character));
    m_defaultValues.insert("scenario-editor/styles-changing/from-action-by-enter", QString::number(ScenarioBlockStyle::SceneHeading));
    m_defaultValues.insert("scenario-editor/styles-changing/from-character-by-tab", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-changing/from-character-by-enter", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-changing/from-dialog-by-tab", QString::number(ScenarioBlockStyle::Parenthetical));
    m_defaultValues.insert("scenario-editor/styles-changing/from-dialog-by-enter", QString::number(ScenarioBlockStyle::Action));
    m_defaultValues.insert("scenario-editor/styles-changing/from-parenthetical-by-tab", QString::number(ScenarioBlockStyle::Dialogue));
    m_defaultValues.insert("scenario-editor/styles-changing/from-parenthetical-by-enter", QString::number(ScenarioBlockStyle::Parenthetical));
    m_defaultValues.insert("scenario-editor/styles-changing/from-transition-by-tab", QString::number(ScenarioBlockStyle::Transition));
    m_defaultValues.insert("scenario-editor/styles-changing/from-transition-by-enter", QString::number(ScenarioBlockStyle::Transition));
    m_defaultValues.insert("scenario-editor/styles-changing/from-note-by-tab", QString::number(ScenarioBlockStyle::Note));
    m_defaultValues.insert("scenario-editor/styles-changing/from-note-by-enter", QString::number(ScenarioBlockStyle::Note));
    m_defaultValues.insert("scenario-editor/styles-changing/from-title-by-tab", QString::number(ScenarioBlockStyle::Title));
    m_defaultValues.insert("scenario-editor/styles-changing/from-title-by-enter", QString::number(ScenarioBlockStyle::Title));
    m_defaultValues.insert("scenario-editor/styles-changing/from-noprintable_text-by-tab", QString::number(ScenarioBlockStyle::NoprintableText));
    m_defaultValues.insert("scenario-editor/styles-changing/from-noprintable_text-by-enter", QString::number(ScenarioBlockStyle::NoprintableText));
    m_defaultValues.insert("scenario-editor/styles-changing/from-folder_header-by-tab", QString::number(ScenarioBlockStyle::FolderHeader));
    m_defaultValues.insert("scenario-editor/styles-changing/from-folder_header-by-enter", QString::number(ScenarioBlockStyle::FolderHeader));
    m_defaultValues.insert("scenario-editor/styles-changing/from-lyrics-by-tab", QString::number(ScenarioBlockStyle::Parenthetical));
    m_defaultValues.insert("scenario-editor/styles-changing/from-lyrics-by-enter", QString::number(ScenarioBlockStyle::Action));
    //
    m_defaultValues.insert("scenario-editor/use-open-bracket-in-dialogue-for-parenthetical", "1");
    m_defaultValues.insert("scenario-editor/review/use-highlight", "1");

    m_defaultValues.insert("chronometry/used", "1");
    m_defaultValues.insert("chronometry/current-chronometer-type", "pages-chronometer");
    m_defaultValues.insert("chronometry/pages/seconds", "60");
    m_defaultValues.insert("chronometry/characters/characters", "1000");
    m_defaultValues.insert("chronometry/characters/seconds", "60");
    m_defaultValues.insert("chronometry/configurable/seconds-for-paragraph/scene_heading", "2");
    m_defaultValues.insert("chronometry/configurable/seconds-for-every-50/scene_heading", "0");
    m_defaultValues.insert("chronometry/configurable/seconds-for-paragraph/action", "1");
    m_defaultValues.insert("chronometry/configurable/seconds-for-every-50/action", "1.5");
    m_defaultValues.insert("chronometry/configurable/seconds-for-paragraph/dialog", "2");
    m_defaultValues.insert("chronometry/configurable/seconds-for-every-50/dialog", "2.4");


    m_defaultValues.insert("counters/pages/used", "0");
    m_defaultValues.insert("counters/words/used", "0");
    m_defaultValues.insert("counters/simbols/used", "0");
}

QVariant SettingsStorage::getCachedValue(const QString& _key, SettingsStorage::SettingsPlace _settingsPlace, bool& _ok)
{
    QVariant result;
    if (_settingsPlace == SettingsStorage::ApplicationSettings) {
        _ok = m_cachedValuesApp.contains(_key);
        result = m_cachedValuesApp.value(_key);
    } else {
        _ok = m_cachedValuesDb.contains(_key);
        result = m_cachedValuesDb.value(_key);
    }
    return result;
}

void SettingsStorage::cacheValue(const QString& _key, const QVariant& _value, SettingsStorage::SettingsPlace _settingsPlace)
{
    if (_settingsPlace == SettingsStorage::ApplicationSettings) {
        m_cachedValuesApp.insert(_key, _value);
    } else {
        m_cachedValuesDb.insert(_key, _value);
    }
}
