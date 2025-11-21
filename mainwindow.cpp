#include "mainwindow.h"
#include "capturesession.h"
#include "ocrservice.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QClipboard>
#include <QMenuBar>
#include <QAction>
#include <QColorDialog>
#include <QFileDialog>
#include <QImage>
#include <QDateTime>

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

void MainWindow::initGUI()
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

    auto settingsMenu = menuBar()->addMenu(tr("Settings"));

    auto colorAct = new QAction(tr("Choose Overlay Color..."), this);
    connect(colorAct, &QAction::triggered, this, [this](){
        m_color = QColorDialog::getColor();
    });
    settingsMenu->addAction(colorAct);

    auto saveScreenshotAct = new QAction(tr("Also Save Screenshot"));
    saveScreenshotAct->setCheckable(true);
    saveScreenshotAct->setChecked(true);
    settingsMenu->addAction(saveScreenshotAct);

    auto selectFolderAct = new QAction(tr("Change Screenshot Save Folder..."));
    connect(selectFolderAct, &QAction::triggered, this, [this](){
        m_dir = QFileDialog::getExistingDirectory();
        // TODO: Implement saving the user-selected folder so that
        // when the program launches next time, it uses the previous selection.
    });

    settingsMenu->addAction(selectFolderAct);

    // This connect() is placed below the action definition because it should go into
    // the menu first, but we also need to access an action that is defined after it in the slot.
    connect(saveScreenshotAct, &QAction::toggled, this, [this, selectFolderAct](bool on){
        m_saveScreenshot = on;
        selectFolderAct->setEnabled(on);
    });

    setWindowTitle("SnipText");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_newShotBtn(nullptr)
    , m_color(QColor("red"))
    , m_saveScreenshot(true)
    , m_ocrService(new OcrService)
{
    m_dir = desktopSavePath();

    initGUI();

    // Create and init Tesseract once.
    if (!m_ocrService->initialize("/opt/homebrew/share/tessdata", "eng")) {
        QMessageBox::critical(this, tr("Tesseract"),
                              tr("Failed to initialize Tesseract. Check tessdata path."));
    }
}

MainWindow::~MainWindow()
{
    delete m_ocrService;
    m_ocrService = nullptr;
}

void MainWindow::onNewScreenshot()
{
    auto *session = createCaptureSession();
    if (!session)
        return;

    session->start();
}

void MainWindow::processCapturedImage(const QImage &image)
{
    // --- OCR (minimal) ---
    if (m_ocrService && m_ocrService->isReady()) {
        const QString text = m_ocrService->extractText(image);
        if (!text.isEmpty()) {
            if (QClipboard *cb = QGuiApplication::clipboard()) {
                cb->setText(text, QClipboard::Clipboard); // Copy to system clipboard.
            }
        }
    }
    // --- end OCR ---

    if (!m_saveScreenshot)
        return;

    const QString fileName =
        QString("snip_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QDir dir(m_dir);
    if (!dir.exists())
        dir.mkpath(".");
    const QString filePath = dir.filePath(fileName);

    if (image.save(filePath, "PNG")) {
        QMessageBox::information(this, tr("Saved"),
                                 tr("Screenshot saved to:\n%1").arg(filePath));
    } else {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to save screenshot to:\n%1").arg(filePath));
    }
}

void MainWindow::handleCaptureError(const QString &errorMessage, bool fatal)
{
    if (errorMessage.isEmpty())
        return;

    if (fatal)
        QMessageBox::critical(this, tr("Error"), errorMessage);
    else
        QMessageBox::warning(this, tr("Warning"), errorMessage);
}

CaptureSession *MainWindow::createCaptureSession()
{
    auto *session = new CaptureSession(this);
    session->setOverlayColor(m_color);
    session->setCaptureDelay(m_captureDelayMs);

    connect(session, &CaptureSession::captureReady,
            this, [this, session](const QImage &image) {
                session->deleteLater();
                processCapturedImage(image);
            });

    connect(session, &CaptureSession::captureFailed,
            this, [this, session](const QString &error, bool fatal) {
                session->deleteLater();
                handleCaptureError(error, fatal);
            });

    return session;
}
