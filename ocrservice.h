#ifndef OCRSERVICE_H
#define OCRSERVICE_H

#include <QString>

class QImage;

namespace tesseract {
class TessBaseAPI;
}

// Thin RAII wrapper that keeps a single TessBaseAPI instance alive for the
// duration of the application.
class OcrService
{
public:
    OcrService();
    ~OcrService();

    // (Re)initialize the engine with the given tessdata path and language.
    bool initialize(const QString &dataPath, const QString &language);
    bool isReady() const;
    // Run OCR on the provided image and return UTF-8 text.
    QString extractText(const QImage &image);

private:
    tesseract::TessBaseAPI *m_api = nullptr;
};

#endif // OCRSERVICE_H
