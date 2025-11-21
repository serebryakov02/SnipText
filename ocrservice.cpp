#include "ocrservice.h"

#include <QImage>

#include <tesseract/baseapi.h>

OcrService::OcrService() = default;

OcrService::~OcrService()
{
    if (m_api) {
        m_api->End(); // Release engine resources.
        delete m_api;
        m_api = nullptr;
    }
}

bool OcrService::initialize(const QString &dataPath, const QString &language)
{
    // Re-create the API so we can change languages or recover from failures.
    if (m_api) {
        m_api->End();
        delete m_api;
        m_api = nullptr;
    }

    m_api = new tesseract::TessBaseAPI();
    if (!m_api)
        return false;

    const QByteArray data = dataPath.toUtf8();
    const QByteArray lang = language.toUtf8();
    if (m_api->Init(data.constData(), lang.constData()) != 0) {
        delete m_api;
        m_api = nullptr;
        return false;
    }

    return true;
}

bool OcrService::isReady() const
{
    return m_api != nullptr;
}

QString OcrService::extractText(const QImage &image)
{
    if (!isReady())
        return {};

    // Tesseract performs best on grayscale data, so convert before feeding it.
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    if (gray.isNull())
        return {};

    m_api->SetImage(gray.constBits(),
                    gray.width(),
                    gray.height(),
                    1,
                    gray.bytesPerLine());

    QString result;
    if (char *utf8 = m_api->GetUTF8Text()) {
        result = QString::fromUtf8(utf8);
        delete [] utf8;

        // Remove page-break leftovers to avoid polluting clipboard text.
        result.remove(QChar::fromLatin1('\f'));
        result = result.trimmed();
    }

    m_api->Clear();
    return result;
}
