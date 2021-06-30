#ifndef BACKUPHELPER_H
#define BACKUPHELPER_H

#include <QString>


/**
 * @brief Класс для организации создания резервных копий
 */
class BackupHelper
{
public:
	BackupHelper();

	/**
	 * @brief Включить/выключить
	 */
	void setIsActive(bool _isActive);

    /**
     * @brief Настроить класс на сохранение тех или иных бекапов. По-умолчанию сохраняются все типы
     */
    void configure(bool _fullBackup, bool _versions);

	/**
	 * @brief Установить папку для сохранения резервных копий
	 */
	void setBackupDir(const QString& _dir);

	/**
	 * @brief Создать резервную копию файла
	 */
    void saveBackup(const QString& _filePath, const QString& _newName = QString());

private:
	/**
	 * @brief Включён/выключен
	 */
	bool m_isActive = true;

    /**
     * @brief Опции сохранения разных видов бэкапов
     */
    bool m_isSaveFullBackup = true;
    bool m_isSaveVersions = true;

	/**
	 * @brief Папка, в которую сохранять резервные копии
	 */
	QString m_backupDir;
};

#endif // BACKUPHELPER_H
