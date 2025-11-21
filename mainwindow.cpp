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
#include <QSettings>

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
    // TODO: Add a keyboard shortcut for this button, let the user customize it,
    // and persist the choice via QSettings.

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
        m_settings->setValue("overlayColor", m_color);
    });
    settingsMenu->addAction(colorAct);

    auto saveScreenshotAct = new QAction(tr("Also Save Screenshot"));
    saveScreenshotAct->setCheckable(true);
    saveScreenshotAct->setChecked(m_saveScreenshot);
    settingsMenu->addAction(saveScreenshotAct);

    auto selectFolderAct = new QAction(tr("Change Screenshot Save Folder..."));
    selectFolderAct->setEnabled(m_saveScreenshot);
    connect(selectFolderAct, &QAction::triggered, this, [this](){
        m_dir = QFileDialog::getExistingDirectory();
        m_settings->setValue("saveFolderPath", m_dir);
    });

    settingsMenu->addAction(selectFolderAct);

    // This connect() is placed below the action definition because it should go into
    // the menu first, but we also need to access an action that is defined after it in the slot.
    connect(saveScreenshotAct, &QAction::toggled, this, [this, selectFolderAct](bool on){
        m_saveScreenshot = on;
        selectFolderAct->setEnabled(on);
        m_settings->setValue("alsoSaveScreenshot", m_saveScreenshot);
    });

    setWindowTitle("SnipText");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_newShotBtn(nullptr)
    , m_color(QColor("red"))
    , m_saveScreenshot(true)
    , m_ocrService(new OcrService)
    , m_settings(new QSettings("MySoft", "SnipText", this))
{
    m_dir = desktopSavePath();

    if (m_settings) {
        const QColor color = m_settings->value("overlayColor").value<QColor>();
        if (color.isValid())
            m_color = color;

        const QString storedDir = m_settings->value("saveFolderPath").toString();
        if (!storedDir.isEmpty())
            m_dir = storedDir;

        if (m_settings->contains("alsoSaveScreenshot"))
            m_saveScreenshot = m_settings->value("alsoSaveScreenshot").toBool();
    }

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
