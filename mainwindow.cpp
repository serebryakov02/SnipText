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

#ifndef DEFAULT_TESSDATA_PATH
#define DEFAULT_TESSDATA_PATH ""
#endif
// DEFAULT_TESSDATA_PATH is injected via CMake so the app knows where tessdata lives
// without hardcoding the path in the source.

static QString desktopSavePath()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (path.isEmpty())
        path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (path.isEmpty())
        path = QDir::currentPath();
    return path;
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
        const QColor chosen = QColorDialog::getColor(m_color, this);
        if (!chosen.isValid())
            return;

        m_color = chosen;
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
        const QString dir = QFileDialog::getExistingDirectory(this, {}, m_dir);
        if (dir.isEmpty())
            return;

        m_dir = dir;
        m_settings->setValue("saveFolderPath", m_dir);
    });

    settingsMenu->addAction(selectFolderAct);

    auto multiAreaAct = new QAction(tr("Capture Multiple Areas"));
    multiAreaAct->setCheckable(true);
    multiAreaAct->setChecked(m_captureMultipleAreas);
    connect(multiAreaAct, &QAction::toggled, this, [this](bool on){
        m_captureMultipleAreas = on;
        m_settings->setValue("multiCaptureEnabled", m_captureMultipleAreas);
    });
    settingsMenu->addAction(multiAreaAct);

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
    , m_captureMultipleAreas(false)
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

        if (m_settings->contains("multiCaptureEnabled"))
            m_captureMultipleAreas = m_settings->value("multiCaptureEnabled").toBool();
    }

    initGUI();

    // Create and init Tesseract once.
    const QString tessdataPath = QString::fromUtf8(DEFAULT_TESSDATA_PATH);
    if (!m_ocrService->initialize(tessdataPath, "eng")) {
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

    if (m_captureMultipleAreas)
        m_multiCaptureTexts.clear();

    session->start();
}

void MainWindow::processCapturedImage(const QImage &image, bool multiCapture)
{
    QString text;
    if (m_ocrService && m_ocrService->isReady())
        text = m_ocrService->extractText(image);

    if (!text.isEmpty()) {
        if (multiCapture) {
            m_multiCaptureTexts.append(text);
        } else {
            if (QClipboard *cb = QGuiApplication::clipboard())
                cb->setText(text, QClipboard::Clipboard);
        }
    }

    if (m_saveScreenshot)
        saveScreenshot(image);
}

void MainWindow::finalizeMultiCapture()
{
    if (m_multiCaptureTexts.isEmpty())
        return;

    const QString finalText = m_multiCaptureTexts.join("\n\n");
    m_multiCaptureTexts.clear();

    if (finalText.isEmpty())
        return;

    if (QClipboard *cb = QGuiApplication::clipboard())
        cb->setText(finalText, QClipboard::Clipboard);
}

void MainWindow::saveScreenshot(const QImage &image)
{
    const QString fileName =
        QString("snip_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QDir dir(m_dir);
    if (!dir.exists())
        dir.mkpath(".");
    const QString filePath = dir.filePath(fileName);

    if (!image.save(filePath, "PNG")) {
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
    session->setMultiSelectionEnabled(m_captureMultipleAreas);

    connect(session, &CaptureSession::captureReady,
            this, [this, session](const QImage &image) {
                const bool multi = session->multiSelectionEnabled();
                processCapturedImage(image, multi);
                if (!multi)
                    session->deleteLater();
            });

    connect(session, &CaptureSession::captureFailed,
            this, [this, session](const QString &error, bool fatal) {
                if (session->multiSelectionEnabled())
                    m_multiCaptureTexts.clear();
                session->deleteLater();
                handleCaptureError(error, fatal);
            });

    connect(session, &CaptureSession::multiCaptureFinished,
            this, [this, session]() {
                finalizeMultiCapture();
                session->deleteLater();
            });

    return session;
}
