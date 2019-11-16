#include "PdfExporter.h"

#include <BusinessLayer/ScenarioDocument/ScenarioDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockInfo.h>

#include <Domain/Scenario.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <3rd_party/Widgets/PagesTextEdit/PageMetrics.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QPdfWriter>
#include <QPointer>
#ifndef MOBILE_OS
#include <QPrinter>
#include <QPrintPreviewDialog>
#endif
#include <QScrollArea>
#include <QScrollBar>
#include <QString>
#include <QtMath>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTimer>

using namespace BusinessLogic;

namespace {
    /**
     * @brief Стиль экспорта
     */
    static ScenarioTemplate exportTemplate() {
        return ScenarioTemplateFacade::getTemplate(
                    DataStorageLayer::StorageFacade::settingsStorage()->value(
                        "export/style",
                        DataStorageLayer::SettingsStorage::ApplicationSettings)
                    );
    }

    /**
     * @brief Есть ли в документе титульная страница
     */
    const char* kPrintTitleKey = "print_title";

    /**
     * @brief Дополнительные параметры печати
     */
    /** @{ */
    const char* kPageHeaderKey = "page_header";
    const char* kPageFooterKey = "page_footer";
    const char* kPrintPageNumbersKey = "page_numbers";
    const char* kPrintScnesNumbersKey = "scenes_numbers";
    const char* kSceneNumbersPrefixKey = "scene_numbers_prefix";
    const char* kPrintDialoguesNumbersKey = "dialogues_numbers";
    /** @} */

    /**
     * @brief Водяной знак
     */
    const char* WATERMARK_KEY = "watermark";

    /**
     * @brief Напечатать страницу документа
     * @note Адаптация функции QTextDocument.cpp::anonymous::printPage
     */
    static void printPage(int _pageNumber, QPainter* _painter, const QTextDocument* _document,
        const QRectF& _body, const QPagedPaintDevice::Margins& _margins)
    {
        const qreal pageYPos = (_pageNumber - 1) * _body.height();

        _painter->save();
        _painter->translate(_body.left(), _body.top() - pageYPos);
        QRectF currentPageRect(0, pageYPos, _body.width(), _body.height());
        QAbstractTextDocumentLayout *layout = _document->documentLayout();
        QAbstractTextDocumentLayout::PaintContext ctx;
        _painter->setClipRect(currentPageRect);
        ctx.clip = currentPageRect;
        // don't use the system palette text as default text color, on HP/UX
        // for example that's white, and white text on white paper doesn't
        // look that nice
        ctx.palette.setColor(QPalette::Text, Qt::black);
        layout->draw(_painter, ctx);

        //
        // Печатаем номера сцен и реплик, если нужно
        //
        const bool needPrintScenesNumbers = _document->property(kPrintScnesNumbersKey).toBool();
        const QString sceneNumbersPrefix = _document->property(kSceneNumbersPrefixKey).toString();
        const bool needPrintDialoguesNumbers = _document->property(kPrintDialoguesNumbersKey).toBool();
        if (needPrintScenesNumbers
            || needPrintDialoguesNumbers) {
            _painter->save();
            //
            // Смещаем пэинтер на ширину поля, чтобы можно было отрисовать номера сцен
            //
            _painter->translate(-1 * PageMetrics::mmToPx(_margins.left), 0);
            const QRectF fullWidthPageRect(0, pageYPos, PageMetrics::mmToPx(_margins.left) + _body.width() + PageMetrics::mmToPx(_margins.right), _body.height());
            _painter->setClipRect(fullWidthPageRect);

            const int blockPos = layout->hitTest(QPointF(0, pageYPos), Qt::FuzzyHit);
            QTextBlock block = _document->findBlock(std::max(0, blockPos));
            while (block.isValid()) {
                const QRectF blockRect = layout->blockBoundingRect(block);
                if (blockRect.bottom() > pageYPos + _body.height()) {
                    break;
                }

                //
                // Покажем номер сцены, если необходимо
                //
                if (ScenarioBlockStyle::forBlock(block) == ScenarioBlockStyle::SceneHeading
                    && !block.text().isEmpty()
                    && needPrintScenesNumbers) {
                    const SceneHeadingBlockInfo* const blockInfo = dynamic_cast<SceneHeadingBlockInfo*>(block.userData());
                    if (blockInfo != nullptr) {
                        QFont font = exportTemplate().blockStyle(ScenarioBlockStyle::SceneHeading).charFormat().font();
                        const qreal fontSize = font.pointSize() / qMin(_painter->viewport().height() / _body.height(),
                                                                       _painter->viewport().width() / _body.width());
                        font.setPointSizeF(fontSize);
                        _painter->setFont(font);
                        //
                        const int distanceBetweenSceneNumberAndText = 10;
                        if (QLocale().textDirection() == Qt::LeftToRight) {
                            const QRectF sceneNumberRect(
                                        0,
                                        blockRect.top() <= pageYPos ? pageYPos : blockRect.top(),
                                        PageMetrics::mmToPx(_margins.left) - distanceBetweenSceneNumberAndText,
                                        blockRect.height());
                            _painter->drawText(sceneNumberRect, Qt::AlignRight | Qt::AlignTop,
                                               sceneNumbersPrefix + blockInfo->sceneNumber() + ".");
                        } else {
                            const QRectF sceneNumberRect(
                                        PageMetrics::mmToPx(_margins.left) + _body.width() + distanceBetweenSceneNumberAndText,
                                        blockRect.top() <= pageYPos ? pageYPos : blockRect.top(),
                                        PageMetrics::mmToPx(_margins.right) - distanceBetweenSceneNumberAndText,
                                        blockRect.height());
                            _painter->drawText(sceneNumberRect, Qt::AlignLeft | Qt::AlignTop,
                                               "." + blockInfo->sceneNumber() + QChar(0x202B) + sceneNumbersPrefix);
                        }
                    }
                }
                //
                // Покажем номер диалога, если необходимо
                //
                else if (ScenarioBlockStyle::forBlock(block) == ScenarioBlockStyle::Character
                         && !block.text().isEmpty()
                         && needPrintDialoguesNumbers) {
                    const CharacterBlockInfo* const blockInfo = dynamic_cast<CharacterBlockInfo*>(block.userData());
                    if (blockInfo != nullptr) {
                        QFont font = exportTemplate().blockStyle(ScenarioBlockStyle::Character).charFormat().font();
                        const qreal fontSize = font.pointSize() / qMin(_painter->viewport().height() / _body.height(),
                                                                       _painter->viewport().width() / _body.width());
                        font.setPointSizeF(fontSize);
                        _painter->setFont(font);
                        //
                        if (QLocale().textDirection() == Qt::LeftToRight) {
                            const QString dialogueNumber = QString("%1:").arg(blockInfo->dialogueNumber());
                            const int numberDelta = _painter->fontMetrics().width(dialogueNumber);
                            QRectF dialogueNumberRect;
                            if (block.blockFormat().leftMargin() > numberDelta) {
                                dialogueNumberRect =
                                        QRectF(
                                            PageMetrics::mmToPx(_margins.left),
                                            blockRect.top() <= pageYPos ? pageYPos : blockRect.top(),
                                            numberDelta,
                                            blockRect.height());
                            } else {
                                const int distanceBetweenSceneNumberAndText = 10;
                                dialogueNumberRect =
                                        QRectF(
                                            0,
                                            blockRect.top() <= pageYPos ? pageYPos : blockRect.top(),
                                            PageMetrics::mmToPx(_margins.left) - distanceBetweenSceneNumberAndText,
                                            blockRect.height());
                            }
                            _painter->drawText(dialogueNumberRect, Qt::AlignRight | Qt::AlignTop, dialogueNumber);
                        } else {
                            const QString dialogueNumber = QString(":%1").arg(blockInfo->dialogueNumber());
                            const int numberDelta = _painter->fontMetrics().width(dialogueNumber);
                            QRectF dialogueNumberRect;
                            if (block.blockFormat().rightMargin() > numberDelta) {
                                dialogueNumberRect =
                                        QRectF(
                                            PageMetrics::mmToPx(_margins.left) + _body.width() - numberDelta,
                                            blockRect.top() <= pageYPos ? pageYPos : blockRect.top(),
                                            numberDelta,
                                            blockRect.height());
                            } else {
                                const int distanceBetweenSceneNumberAndText = 10;
                                dialogueNumberRect =
                                        QRectF(
                                            PageMetrics::mmToPx(_margins.left) + _body.width() + distanceBetweenSceneNumberAndText,
                                            blockRect.top() <= pageYPos ? pageYPos : blockRect.top(),
                                            PageMetrics::mmToPx(_margins.right) - distanceBetweenSceneNumberAndText,
                                            blockRect.height());
                            }
                            _painter->drawText(dialogueNumberRect, Qt::AlignRight | Qt::AlignTop, dialogueNumber);
                        }
                    }
                }

                block = block.next();
            }
            _painter->restore();
        }

        //
        // Если необходимо рисуем нумерацию страниц
        //
        const bool needPrintPageNumbers = _document->property(kPrintPageNumbersKey).toBool();
        const bool needPrintTitlePage = _document->property(kPrintTitleKey).toBool();
        if (needPrintPageNumbers) {
            //
            // На титульной и на первой странице сценария
            //
            if ((needPrintTitlePage && _pageNumber < 3)
                || _pageNumber == 1) {
                //
                // ... не печатаем номер
                //
            }
            //
            // Печатаем номера страниц
            //
            else {
                _painter->save();
                QFont font = exportTemplate().blockStyle(ScenarioBlockStyle::Action).charFormat().font();
                const qreal fontSize = font.pointSize() / qMin(_painter->viewport().height() / _body.height(),
                                                               _painter->viewport().width() / _body.width());
                font.setPointSizeF(fontSize);
                _painter->setFont(font);

                //
                // Середины верхнего и нижнего полей
                //
                qreal headerY = pageYPos - PageMetrics::mmToPx(_margins.top) / 2;
                qreal footerY = pageYPos + currentPageRect.height() + PageMetrics::mmToPx(_margins.bottom) / 2;

                //
                // Области для прорисовки текста на полях
                //
                QRectF headerRect(0, headerY, currentPageRect.width(), 20);
                QRectF footerRect(0, footerY - 20, currentPageRect.width(), 20);

                //
                // Определяем где положено находиться нумерации
                //
                QRectF numberingRect;
                if (exportTemplate().numberingAlignment().testFlag(Qt::AlignTop)) {
                    numberingRect = headerRect;
                } else {
                    numberingRect = footerRect;
                }
                Qt::Alignment numberingAlignment = Qt::AlignVCenter;
                if (exportTemplate().numberingAlignment().testFlag(Qt::AlignLeft)) {
                    numberingAlignment |= Qt::AlignLeft;
                } else if (exportTemplate().numberingAlignment().testFlag(Qt::AlignCenter)) {
                    numberingAlignment |= Qt::AlignCenter;
                } else {
                    numberingAlignment |= Qt::AlignRight;
                }

                //
                // Рисуем нумерацию в положеном месте (отнимаем единицу, т.к. нумерация
                // должна следовать с единицы для первой страницы текста сценария)
                //
                int titleDelta = needPrintTitlePage ? -1 : 0;
                _painter->setClipRect(numberingRect);
                _painter->drawText(
                    numberingRect, numberingAlignment,
                    QString(QLocale().textDirection() == Qt::LeftToRight ? "%1." : ".%1").arg(_pageNumber + titleDelta));
                _painter->restore();
            }
        }

        //
        // Печатаем колонтитулы, если необходимо
        //
        const QString pageHeader = _document->property(kPageHeaderKey).toString();
        const QString pageFooter = _document->property(kPageFooterKey).toString();
        if (!pageHeader.isEmpty() || !pageFooter.isEmpty()) {
            //
            // пропускаем титульную страницу
            //
            if (needPrintTitlePage && _pageNumber == 1) {
                //
                // ... не печатаем колонтитулы
                //
            }
            //
            // Печатаем колонтитулы
            //
            else {
                _painter->save();
                QFont font = exportTemplate().blockStyle(ScenarioBlockStyle::Action).charFormat().font();
                const qreal fontSize = font.pointSize() / qMin(_painter->viewport().height() / _body.height(),
                                                               _painter->viewport().width() / _body.width());
                font.setPointSizeF(fontSize);
                _painter->setFont(font);

                //
                // Середины верхнего и нижнего полей
                //
                qreal headerY = pageYPos - PageMetrics::mmToPx(_margins.top) / 2;
                qreal footerY = pageYPos + currentPageRect.height() + PageMetrics::mmToPx(_margins.bottom) / 2;

                //
                // Области для прорисовки текста на полях
                //
                QRectF headerRect(0, headerY, currentPageRect.width(), 20);
                QRectF footerRect(0, footerY - 20, currentPageRect.width(), 20);

                //
                // Определяем где положено находиться нумерации
                //
                Qt::Alignment headerAlignment = Qt::AlignVCenter;
                Qt::Alignment footerAlignment = Qt::AlignVCenter;
                if (exportTemplate().numberingAlignment().testFlag(Qt::AlignTop)) {
                    if (exportTemplate().numberingAlignment().testFlag(Qt::AlignLeft)) {
                        headerAlignment |= Qt::AlignRight;
                    } else {
                        headerAlignment |= Qt::AlignLeft;
                    }
                } else {
                    if (exportTemplate().numberingAlignment().testFlag(Qt::AlignLeft)) {
                        footerAlignment |= Qt::AlignRight;
                    } else {
                        footerAlignment |= Qt::AlignLeft;
                    }
                }

                //
                // Рисуем колонтитулы
                //
                _painter->setClipRect(headerRect);
                _painter->drawText(headerRect, headerAlignment, pageHeader);
                _painter->setClipRect(footerRect);
                _painter->drawText(footerRect, footerAlignment, pageFooter);
                _painter->restore();
            }
        }

        _painter->restore();

        //
        // Рисуем водяные знаки
        //
        const QString watermark = "  " + _document->property(WATERMARK_KEY).toString() + "    ";
        if (!watermark.isEmpty()) {
            _painter->save();

            //
            // Рассчитаем какого размера нужен шрифт
            //
            QFont font;
            font.setBold(true);
            font.setPixelSize(400);
            const int maxWidth = static_cast<int>(sqrt(pow(_body.height(), 2) + pow(_body.width(), 2)));
            QFontMetrics fontMetrics(font);
            while (fontMetrics.width(watermark) > maxWidth) {
                font.setPixelSize(font.pixelSize() - 2);
                fontMetrics = QFontMetrics(font);
            }

            _painter->setFont(font);
            _painter->setPen(QColor(100, 100, 100, 100));

            //
            // Рисуем водяной знак
            //
            _painter->rotate(qRadiansToDegrees(atan(_body.height() / _body.width())));
            const int delta = fontMetrics.height() / 4;
            _painter->drawText(delta, delta, watermark);

            //
            // TODO: Рисуем мусор на странице, чтобы текст нельзя было вытащить
            //

            _painter->restore();
        }
    }

#ifndef MOBILE_OS
    /**
     * @brief Напечатать документ
     * @note Адаптация функции QTextDocument::print
     */
    static void printDocument(QTextDocument* _document, QPrinter* _printer)
    {
        QPainter painter(_printer);
        // Check that there is a valid device to print to.
        if (!painter.isActive())
            return;
        QScopedPointer<QTextDocument> clonedDoc;
        (void)_document->documentLayout(); // make sure that there is a layout
        QRectF body = QRectF(QPointF(0, 0), _document->pageSize());

        {
            qreal sourceDpiX = painter.device()->logicalDpiX();
            qreal sourceDpiY = sourceDpiX;
            QPaintDevice *dev = _document->documentLayout()->paintDevice();
            if (dev) {
                sourceDpiX = dev->logicalDpiX();
                sourceDpiY = dev->logicalDpiY();
            }
            const qreal dpiScaleX = qreal(_printer->logicalDpiX()) / sourceDpiX;
            const qreal dpiScaleY = qreal(_printer->logicalDpiY()) / sourceDpiY;
            // scale to dpi
            painter.scale(dpiScaleX, dpiScaleY);
            QSizeF scaledPageSize = _document->pageSize();
            scaledPageSize.rwidth() *= dpiScaleX;
            scaledPageSize.rheight() *= dpiScaleY;
            const QSizeF printerPageSize(_printer->pageRect().size());
            // scale to page
            painter.scale(printerPageSize.width() / scaledPageSize.width(),
                    printerPageSize.height() / scaledPageSize.height());
        }

        int docCopies;
        int pageCopies;
        if (_printer->collateCopies() == true){
            docCopies = 1;
            pageCopies = _printer->supportsMultipleCopies() ? 1 : _printer->copyCount();
        } else {
            docCopies = _printer->supportsMultipleCopies() ? 1 : _printer->copyCount();
            pageCopies = 1;
        }
        int fromPage = _printer->fromPage();
        int toPage = _printer->toPage();
        bool ascending = true;
        if (fromPage == 0 && toPage == 0) {
            fromPage = 1;
            toPage = _document->pageCount();
        }
        // paranoia check
        fromPage = qMax(1, fromPage);
        toPage = qMin(_document->pageCount(), toPage);
        if (toPage < fromPage) {
            // if the user entered a page range outside the actual number
            // of printable pages, just return
            return;
        }
        if (_printer->pageOrder() == QPrinter::LastPageFirst) {
            int tmp = fromPage;
            fromPage = toPage;
            toPage = tmp;
            ascending = false;
        }
        for (int i = 0; i < docCopies; ++i) {
            int page = fromPage;
            while (true) {
                for (int j = 0; j < pageCopies; ++j) {
                    if (_printer->printerState() == QPrinter::Aborted
                        || _printer->printerState() == QPrinter::Error)
                        return;
                    printPage(page, &painter, _document, body, _printer->margins());
                    if (j < pageCopies - 1)
                        _printer->newPage();
                }
                if (page == toPage)
                    break;
                if (ascending)
                    ++page;
                else
                    --page;
                _printer->newPage();
            }
            if ( i < docCopies - 1)
                _printer->newPage();
        }
    }

    /**
     * @brief Прокрутить диалог предпросмотра к заданной позиции
     */
    static bool setPrintPreviewScrollValue(QSharedPointer<QPrintPreviewDialog> _dialog, int _value) {
        if (_dialog.isNull()) {
            return false;
        }

        for (QAbstractScrollArea* child : _dialog->findChildren<QAbstractScrollArea*>()) {
            if (QString(child->metaObject()->className()) == "GraphicsView") {
                child->verticalScrollBar()->setValue(_value);
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Получить позиции прокрутки диалога предпросмотра печати
     */
    static int printPreviewScrollValue(QSharedPointer<QPrintPreviewDialog> _dialog) {
        if (_dialog.isNull()) {
            return 0;
        }

        for (const QAbstractScrollArea* child : _dialog->findChildren<QAbstractScrollArea*>()) {
            if (QString(child->metaObject()->className()) == "GraphicsView") {
                return child->verticalScrollBar()->value();
            }
        }
        return 0;
    }
#else
    /**
     * @brief Напечатать документ
     * @note Адаптация функции QTextDocument::print
     */
    static void printDocument(QTextDocument* _document, QPdfWriter* _printer)
    {
        QPainter painter(_printer);
        // Check that there is a valid device to print to.
        if (!painter.isActive())
            return;
        QScopedPointer<QTextDocument> clonedDoc;
        (void)_document->documentLayout(); // make sure that there is a layout
        QRectF body = QRectF(QPointF(0, 0), _document->pageSize());

        {
            qreal sourceDpiX = painter.device()->logicalDpiX();
            qreal sourceDpiY = sourceDpiX;
            QPaintDevice *dev = _document->documentLayout()->paintDevice();
            if (dev) {
                sourceDpiX = dev->logicalDpiX();
                sourceDpiY = dev->logicalDpiY();
            }
            const qreal dpiScaleX = qreal(_printer->logicalDpiX()) / sourceDpiX;
            const qreal dpiScaleY = qreal(_printer->logicalDpiY()) / sourceDpiY;
            // scale to dpi
            painter.scale(dpiScaleX, dpiScaleY);
            QSizeF scaledPageSize = _document->pageSize();
            scaledPageSize.rwidth() *= dpiScaleX;
            scaledPageSize.rheight() *= dpiScaleY;
            const QSizeF printerPageSize(painter.viewport().size());
            // scale to page
            painter.scale(printerPageSize.width() / scaledPageSize.width(),
                    printerPageSize.height() / scaledPageSize.height());
        }

        int docCopies = 1;
        int pageCopies = 1;
        int fromPage = 1;
        int toPage = _document->pageCount();
        bool ascending = true;
        // paranoia check
        fromPage = qMax(1, fromPage);
        toPage = qMin(_document->pageCount(), toPage);
        for (int i = 0; i < docCopies; ++i) {
            int page = fromPage;
            while (true) {
                for (int j = 0; j < pageCopies; ++j) {
                    printPage(page, &painter, _document, body, _printer->margins());
                    if (j < pageCopies - 1)
                        _printer->newPage();
                }
                if (page == toPage)
                    break;
                if (ascending)
                    ++page;
                else
                    --page;
                _printer->newPage();
            }
            if ( i < docCopies - 1)
                _printer->newPage();
        }
    }
#endif
}


QPair<ScenarioDocument*, int> PdfExporter::m_lastScriptPreviewScrollPosition;

PdfExporter::PdfExporter(QObject* _parent) :
    QObject(_parent),
    AbstractExporter()
{
}

PdfExporter::~PdfExporter()
{
}

void PdfExporter::exportTo(ScenarioDocument* _scenario, const ExportParameters& _exportParameters) const
{
    //
    // Настроим принтер
    //
#ifndef MOBILE_OS
    QScopedPointer<QPrinter> printer(preparePrinter(_exportParameters.filePath));
#else
    QScopedPointer<QPdfWriter> printer(new QPdfWriter(_exportParameters.filePath));
    printer->setPageSize(QPageSize(::exportTemplate().pageSizeId()));
    QMarginsF margins = ::exportTemplate().pageMargins();
    printer->setPageMargins(margins, QPageLayout::Millimeter);
#endif

    //
    // Сформируем документ
    //
    ExportParameters fakeParameters = _exportParameters;
    fakeParameters.printScenesNumbers = false;
    fakeParameters.printDialoguesNumbers = false;
    QTextDocument* preparedDocument = prepareDocument(_scenario, fakeParameters);
    //
    // ... настроим документ для печати
    //
    preparedDocument->setProperty(kPrintTitleKey, _exportParameters.printTilte);
    preparedDocument->setProperty(kPageHeaderKey, _exportParameters.scriptHeader);
    preparedDocument->setProperty(kPageFooterKey, _exportParameters.scriptFooter);
    preparedDocument->setProperty(kPrintPageNumbersKey, _exportParameters.printPagesNumbers);
    preparedDocument->setProperty(kSceneNumbersPrefixKey, _exportParameters.scenesPrefix);
    preparedDocument->setProperty(kPrintScnesNumbersKey, _exportParameters.printScenesNumbers);
    preparedDocument->setProperty(kPrintDialoguesNumbersKey, _exportParameters.printDialoguesNumbers);
    if (_exportParameters.printWatermark) {
        preparedDocument->setProperty(WATERMARK_KEY, _exportParameters.watermark);
    }

    //
    // Печатаем документ
    //
    ::printDocument(preparedDocument, printer.data());
}

void PdfExporter::exportTo(const ResearchModelCheckableProxy* _researchModel, const ExportParameters& _exportParameters) const
{
    //
    // Настроим принтер
    //
#ifndef MOBILE_OS
    QScopedPointer<QPrinter> printer(preparePrinter(_exportParameters.filePath));
#else
    QScopedPointer<QPdfWriter> printer(new QPdfWriter(_exportParameters.filePath));
    printer->setPageSize(QPageSize(::exportTemplate().pageSizeId()));
    QMarginsF margins = ::exportTemplate().pageMargins();
    printer->setPageMargins(margins, QPageLayout::Millimeter);
#endif

    //
    // Сформируем документ
    //
    QTextDocument* preparedDocument = prepareDocument(_researchModel, _exportParameters);
    preparedDocument->setProperty(kPrintTitleKey, false);
    preparedDocument->setProperty(kPrintPageNumbersKey, true);

    //
    // Печатаем документ
    //
    ::printDocument(preparedDocument, printer.data());
}

#ifndef MOBILE_OS
void PdfExporter::printPreview(ScenarioDocument* _scenario, const ExportParameters& _exportParameters)
{
    //
    // Настроим принтер
    //
    QScopedPointer<QPrinter> printer(preparePrinter());

    //
    // Сформируем документ
    //
    ExportParameters fakeParameters = _exportParameters;
    fakeParameters.printScenesNumbers = false;
    fakeParameters.printDialoguesNumbers = false;
    QScopedPointer<QTextDocument> preparedDocument(prepareDocument(_scenario, fakeParameters));
    //
    // ... настроим документ для печати
    //
    preparedDocument->setProperty(kPrintTitleKey, _exportParameters.printTilte);
    preparedDocument->setProperty(kPageHeaderKey, _exportParameters.scriptHeader);
    preparedDocument->setProperty(kPageFooterKey, _exportParameters.scriptFooter);
    preparedDocument->setProperty(kPrintPageNumbersKey, _exportParameters.printPagesNumbers);
    preparedDocument->setProperty(kSceneNumbersPrefixKey, _exportParameters.scenesPrefix);
    preparedDocument->setProperty(kPrintScnesNumbersKey, _exportParameters.printScenesNumbers);
    preparedDocument->setProperty(kPrintDialoguesNumbersKey, _exportParameters.printDialoguesNumbers);
    if (_exportParameters.printWatermark) {
        preparedDocument->setProperty(WATERMARK_KEY, _exportParameters.watermark);
    }

    //
    // Сохраним указатель на документ для печати
    //
    m_documentForPrint.swap(preparedDocument);

    //
    // Настроим диалог предварительного просмотра
    //
    QSharedPointer<QPrintPreviewDialog> printDialog(new QPrintPreviewDialog(printer.data(), qApp->activeWindow()));
    printDialog->setWindowState(Qt::WindowMaximized);
    connect(printDialog.data(), &QPrintPreviewDialog::paintRequested, this, &PdfExporter::aboutPrint);
    if (m_lastScriptPreviewScrollPosition.first == _scenario) {
        //
        // Если осуществляется повторный предпросмотр документа, пробуем восстановить положение полосы прокрутки
        //
        connect(this, &PdfExporter::printed, printDialog.data(), [printDialog] {
            QTimer::singleShot(300, [printDialog] {
                setPrintPreviewScrollValue(printDialog, m_lastScriptPreviewScrollPosition.second);
            });
        });
    } else {
        m_lastScriptPreviewScrollPosition.first = _scenario;
    }

    //
    // Вызываем диалог предварительного просмотра и печати
    //
    printDialog->exec();

    //
    // Сохраняем позицию прокрутки
    //
    m_lastScriptPreviewScrollPosition.second = printPreviewScrollValue(printDialog);
    printDialog->disconnect();
}

void PdfExporter::printPreview(const ResearchModelCheckableProxy* _researchModel, const ExportParameters& _exportParameters)
{
    //
    // Настроим принтер
    //
    QScopedPointer<QPrinter> printer(preparePrinter());

    //
    // Сформируем документ
    //
    QScopedPointer<QTextDocument> preparedDocument(prepareDocument(_researchModel, _exportParameters));
    preparedDocument->setProperty(kPrintTitleKey, false);
    preparedDocument->setProperty(kPrintPageNumbersKey, true);

    //
    // Сохраним указатель на документ для печати
    //
    m_documentForPrint.swap(preparedDocument);

    //
    // Настроим диалог предварительного просмотра
    //
    QSharedPointer<QPrintPreviewDialog> printDialog(new QPrintPreviewDialog(printer.data(), qApp->activeWindow()));
    printDialog->setWindowState(Qt::WindowMaximized);
    connect(printDialog.data(), &QPrintPreviewDialog::paintRequested, this, &PdfExporter::aboutPrint);

    //
    // Вызываем диалог предварительного просмотра и печати
    //
    printDialog->exec();

    //
    // Сохраняем позицию прокрутки
    //
    m_lastScriptPreviewScrollPosition.second = printPreviewScrollValue(printDialog);
    printDialog->disconnect();
}

void PdfExporter::aboutPrint(QPrinter* _printer)
{
    printDocument(m_documentForPrint.data(), _printer);

    emit printed();
}

QPrinter* PdfExporter::preparePrinter(const QString& _forFile) const
{
    QPrinter* printer = new QPrinter;
    printer->setPageSize(QPageSize(::exportTemplate().pageSizeId()));
    QMarginsF margins = ::exportTemplate().pageMargins();
    printer->setPageMargins(margins.left(), margins.top(), margins.right(), margins.bottom(),
                            QPrinter::Millimeter);

    if (!_forFile.isNull()) {
        printer->setOutputFileName(_forFile);
        printer->setOutputFormat(QPrinter::PdfFormat);
    }

    return printer;
}
#endif
