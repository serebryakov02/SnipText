#ifndef OCRSERVICE_H
#define OCRSERVICE_H

#include <QString>

class QImage;

namespace tesseract {
class TessBaseAPI;
}

class OcrService
{
public:
    OcrService();
    ~OcrService();

    bool initialize(const QString &dataPath, const QString &language);
    bool isReady() const;
    QString extractText(const QImage &image);

private:
    tesseract::TessBaseAPI *m_api = nullptr;
};

#endif // OCRSERVICE_H
