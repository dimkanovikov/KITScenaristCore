#ifndef STYLESHEETHELPER_H
#define STYLESHEETHELPER_H

#include <QApplication>
#include <QString>
#include <QRegularExpression>

#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#else
#include <QScreen>
#endif


/**
 * @brief Вспомогательный класс для работы со стилями
 */
class StyleSheetHelper
{
public:
	/**
	 * @brief Обновить стиль, изменив все метрики на девайсонезависимые пиксели
	 * @note Метрики к замене должны быть указаны как 24dp
	 */
	static QString computeDeviceInpedendentSize(const QString& _styleSheet) {
		//
		// Заменяем все единицы на корректные в пикселах для текущего устройства
		//
		const QString oneLineStyleSheet = _styleSheet.simplified();
		QRegularExpression re("(\\d{1,})dp");
		QRegularExpressionMatch match = re.match(oneLineStyleSheet);
		int matchOffset = 0;
		QString resultStyleSheet = _styleSheet;
		while (match.hasMatch()) {
			const QString sourceValue = match.captured();
			const int targetValueInDp = match.captured(1).toInt();
			const QString targetValue = QString("%1px").arg(dpToPx(targetValueInDp));
			resultStyleSheet.replace(sourceValue, targetValue);

			matchOffset = match.capturedEnd();
			match = re.match(oneLineStyleSheet, matchOffset);
		}
		return resultStyleSheet;
	}

	/**
	 * @brief Преобразовать значение в девайсонезависимые пиксели
	 */
	static qreal dpToPx(int _pixelValue) {
        return _pixelValue * dp();
	}

    /**
     * @brief Является ли устройство планшетом
     */
    static bool deviceIsTablet() {
        return isTablet();
    }

private:
	/**
     * @brief Коэффициент девайсонезависимых пикселей
	 */
	static qreal dp() {
        static qreal s_dp = init(true);
		return s_dp;
    }

    /**
     * @brief Является ли устройство планшетом
     */
    static bool isTablet() {
        static bool s_isTablet = init(false) > 6.;
        return s_isTablet;
    }

    /**
     * @brief Определить dp (true) или размер диагонали экрана (false)
     */
    static qreal init(bool _initDp) {
        //
        // Рассчитаем dp и определяем размер устройства
        //
#ifdef Q_OS_ANDROID
        //  BUG with dpi on some androids: https://bugreports.qt-project.org/browse/QTBUG-35701
        //  Workaround:
        QAndroidJniObject qtActivity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
        QAndroidJniObject resources = qtActivity.callObjectMethod("getResources", "()Landroid/content/res/Resources;");
        QAndroidJniObject displayMetrics = resources.callObjectMethod("getDisplayMetrics", "()Landroid/util/DisplayMetrics;");
        const int width = displayMetrics.getField<int>("widthPixels");
        const int height = displayMetrics.getField<int>("heightPixels");
        const int density = displayMetrics.getField<int>("densityDpi");
#else
        QScreen *screen = QApplication::primaryScreen();
        float density = screen->physicalDotsPerInch();
        const int width = screen->geometry().width();
        const int height = screen->geometry().height();
#endif
        if (_initDp) {
            qreal dp =
                density < 180 ? 1
                : density < 270 ? 1.5
                : density < 360 ? 2 : 3;
            return dp;
        } else {
            const qreal w = width / density;
            const qreal h = height /density;
            const qreal d = sqrt(w*w + h*h);
            return d;
        }
    }
};

#endif // STYLESHEETHELPER_H
