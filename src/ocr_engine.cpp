#include "ocr_engine.h"

#include <tesseract/baseapi.h>
#include <stdexcept>

OcrEngine::OcrEngine() : api_(new tesseract::TessBaseAPI())
{
    if (api_->Init(nullptr, "eng") != 0)
    {
        delete api_;
        throw std::runtime_error("Failed to initialize Tesseract (is tesseract-ocr-eng installed?)");
    }
    // Regions are expected to hold a single short line of text; this mode
    // is both more accurate and faster than full page segmentation for that.
    api_->SetPageSegMode(tesseract::PSM_SINGLE_LINE);
}

OcrEngine::~OcrEngine()
{
    api_->End();
    delete api_;
}

OcrResult OcrEngine::recognize(const cv::Mat &gray)
{
    api_->SetImage(gray.data, gray.cols, gray.rows, 1, static_cast<int>(gray.step));

    OcrResult result;
    char *raw_text = api_->GetUTF8Text();
    if (raw_text != nullptr)
    {
        result.text = raw_text;
        while (!result.text.empty() && (result.text.back() == '\n' || result.text.back() == '\r'))
        {
            result.text.pop_back();
        }
        delete[] raw_text;
    }
    result.confidence = static_cast<float>(api_->MeanTextConf());
    return result;
}
