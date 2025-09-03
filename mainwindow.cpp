#include "mainwindow.h"
#include "selectionoverlay.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QCursor>
#include <QMessageBox>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QClipboard>

#include <tesseract/baseapi.h>

namespace {
QString desktopSavePath()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (path.isEmpty())
        path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (path.isEmpty())
        path = QDir::currentPath();
    return path;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_newShotBtn(nullptr)
    , m_screen(nullptr)
{
    auto *cw = new QWidget(this);
    setCentralWidget(cw);

    m_newShotBtn = new QPushButton(tr("New Screenshot"), cw);

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_newShotBtn, 0, Qt::AlignCenter);
    layout->addStretch(1);
    cw->setLayout(layout);

    connect(m_newShotBtn, &QPushButton::clicked,
            this, &MainWindow::onNewScreenshot);

    // Create and init Tesseract once.
    m_ocr = new tesseract::TessBaseAPI();
    const int initRc = m_ocr->Init("/opt/homebrew/share/tessdata", "eng");
    if (initRc != 0) {
        QMessageBox::critical(this, tr("Tesseract"),
                              tr("Failed to initialize Tesseract. Check tessdata path."));
        delete m_ocr;
        m_ocr = nullptr;
    }

    setWindowTitle("SnipText");
}

MainWindow::~MainWindow()
{
    if (m_ocr) {
        m_ocr->End(); // Release engine resources.
        delete m_ocr;
        m_ocr = nullptr;
    }
}

void MainWindow::onNewScreenshot()
{
    // Choose and store the screen under the cursor; fall back to primary.
    m_screen = QGuiApplication::screenAt(QCursor::pos());
    if (!m_screen)
        m_screen = QGuiApplication::primaryScreen();

    if (!m_screen) {
        QMessageBox::critical(this, tr("Error"), tr("No screen available."));
        return;
    }

    // Create a transient overlay (top-level window).
    auto *overlay = new SelectionOverlay;
    overlay->setAttribute(Qt::WA_DeleteOnClose, true);
    overlay->setGeometry(m_screen->geometry());
    overlay->show();

    // When selection finishes, hide overlay, defer grab, then save.
    connect(overlay, &SelectionOverlay::selectionFinished,
            this, [this, overlay](const QRect &logicalRect) {
                if (logicalRect.isNull())
                    return;

                // Ensure overlay is not captured.
                overlay->hide();

                // Give the compositor time to remove it from the frame.
                QTimer::singleShot(m_captureDelayMs, this, [this, logicalRect]() {
                    saveSelectionFromScreen(logicalRect);
                });
            });

    // If user cancels, just close/hide the overlay.
    connect(overlay, &SelectionOverlay::selectionCanceled,
            overlay, &SelectionOverlay::close);
}

void MainWindow::saveSelectionFromScreen(const QRect &selectionLogical)
{
    // Use the session's screen (selected in onNewScreenshot()).
    if (!m_screen) {
        QMessageBox::critical(this, tr("Error"), tr("No screen available."));
        return;
    }

    const QPixmap snap = m_screen->grabWindow(0);
    if (snap.isNull()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to capture the screen."));
        return;
    }

    // The selection rectangle comes from the overlay in "logical" coordinates (DPI-independent).
    // The screenshot pixmap uses real screen pixels. On Retina/HiDPI displays, 1 logical unit
    // can equal 2 or more physical pixels. Multiply by devicePixelRatio (dpr) to match them.
    const qreal dpr = snap.devicePixelRatio();
    const QRect pixelRect(
        qRound(selectionLogical.x()      * dpr),
        qRound(selectionLogical.y()      * dpr),
        qRound(selectionLogical.width()  * dpr),
        qRound(selectionLogical.height() * dpr)
        );

    const QImage src = snap.toImage(); // Device pixels.
    if (!src.rect().contains(pixelRect)) {
        QMessageBox::warning(this, tr("Warning"), tr("Selection is out of bounds."));
        return;
    }

    const QImage cropped = src.copy(pixelRect);

    // --- OCR (minimal) ---
    if (m_ocr) {
        QImage gray = cropped.convertToFormat(QImage::Format_Grayscale8);
        if (!gray.isNull()) {
            m_ocr->SetImage(gray.constBits(),
                            gray.width(),
                            gray.height(),
                            1,
                            gray.bytesPerLine());

            char* utf8 = m_ocr->GetUTF8Text();
            if (utf8) {
                QString out = QString::fromUtf8(utf8);
                delete [] utf8;

                out.remove(QChar::fromLatin1('\f'));
                out = out.trimmed();
                if (!out.isEmpty()) {
                    if (QClipboard* cb = QGuiApplication::clipboard()) {
                        cb->setText(out, QClipboard::Clipboard); // Copy to system clipboard.
                    }
                }

                m_ocr->Clear();
            }
        }
    }
    // --- end OCR ---

    const QString baseDir = desktopSavePath();
    const QString fileName =
        QString("snip_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString filePath = QDir(baseDir).filePath(fileName);

    if (cropped.save(filePath, "PNG")) {
        QMessageBox::information(this, tr("Saved"),
                                 tr("Screenshot saved to:\n%1").arg(filePath));
    } else {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to save screenshot to:\n%1").arg(filePath));
    }
}
