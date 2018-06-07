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

    //
    // Блокирующие цыкл событий методы
    //

    static QDialogButtonBox::StandardButton critical(QWidget* _parent, const QString& _title,
		const QString& _text, QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonInfos = QVector<ButtonInfo>());

	static QDialogButtonBox::StandardButton information(QWidget* _parent, const QString& _title,
		const QString& _text, QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonInfos = QVector<ButtonInfo>());

	static QDialogButtonBox::StandardButton question(QWidget* _parent, const QString& _title, const QString& _text,
		QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::StandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No),
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonInfos = QVector<ButtonInfo>());

	static QDialogButtonBox::StandardButton warning(QWidget* _parent, const QString& _title,
		const QString& _text, QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonInfos = QVector<ButtonInfo>());

    //
    // Не блокирующие цыкл событий методы
    //

    static QLightBoxMessage* showCritical(QWidget* _parent, const QString& _title,
        const QString& _text, QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonInfos = QVector<ButtonInfo>());

    static QLightBoxMessage* showInformation(QWidget* _parent, const QString& _title,
        const QString& _text, QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonInfos = QVector<ButtonInfo>());

    static QLightBoxMessage* showQuestion(QWidget* _parent, const QString& _title, const QString& _text,
        QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::StandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No),
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonInfos = QVector<ButtonInfo>());

    static QLightBoxMessage* showWarning(QWidget* _parent, const QString& _title,
        const QString& _text, QDialogButtonBox::StandardButtons _buttons = QDialogButtonBox::Ok,
        QDialogButtonBox::StandardButton _defaultButton = QDialogButtonBox::NoButton,
        const QVector<ButtonInfo>& _buttonInfos = QVector<ButtonInfo>());

private:
    /**
     * @brief Выполнить подготовленное сообщение и вернуть результат
     */
    static QDialogButtonBox::StandardButton execMessage(QLightBoxMessage* _message);

    /**
     * @brief Показать подготовленное сообщение неблокирующим способом
     */
    static QLightBoxMessage* showMessage(QLightBoxMessage* _message);

    /**
     * @brief Подготовить сообщение
     */
    static QLightBoxMessage* prepareMessage(QWidget* _parent, const QString& _title,
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
