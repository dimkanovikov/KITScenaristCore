#ifndef QLIGHTBOXMESSAGE_H
#define QLIGHTBOXMESSAGE_H

#include "qlightboxdialog.h"

#include <QDialogButtonBox>
#include <QStyle>


/**
 * @brief Фабрика сообщений
 */
class QLightBoxMessage : public QLightBoxDialog
{
	Q_OBJECT

public:
    struct ButtonInfo {
        QDialogButtonBox::StandardButton type;
        QString name;
    };

    static QDialogButtonBox::StandardButton critical(QWidget* _parent, const QString& _title,
		const QString& _text, QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonNames = QVector<ButtonInfo>());

	static QDialogButtonBox::StandardButton information(QWidget* _parent, const QString& _title,
		const QString& _text, QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonNames = QVector<ButtonInfo>());

	static QDialogButtonBox::StandardButton question(QWidget* _parent, const QString& _title, const QString& _text,
		QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::StandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No),
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonNames = QVector<ButtonInfo>());

	static QDialogButtonBox::StandardButton warning(QWidget* _parent, const QString& _title,
		const QString& _text, QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonNames = QVector<ButtonInfo>());

private:
	static QDialogButtonBox::StandardButton message(QWidget* _parent, const QString& _title,
		const QString& _text, QStyle::StandardPixmap _pixmap, QDialogButtonBox::StandardButtons _buttons,
        QDialogButtonBox::StandardButton _defaultButton, const QVector<ButtonInfo>& _buttonNames);

private:
    explicit QLightBoxMessage(QWidget* _parent = nullptr);

	/**
	 * @brief Настроить представление
	 */
    void initView() override;

	/**
	 * @brief Настроить соединения
	 */
    void initConnections() override;

	/**
	 * @brief Устанавливаем фокус на кнопки
	 */
    QWidget* focusedOnExec() const override;

private slots:
	/**
	 * @brief Нажата кнопка
	 */
	void aboutClicked(QAbstractButton* _button);

private:
	/**
	 * @brief Иконка
	 */
	QLabel* m_icon;

	/**
	 * @brief Текст
	 */
	QLabel* m_text;

	/**
	 * @brief Кнопки диалога
	 */
	QDialogButtonBox* m_buttons;
};

#endif // QLIGHTBOXMESSAGE_H
