#pragma once
#include <opencv2/core.hpp>
#include <string>

namespace tesseract
{
    class TessBaseAPI;
}

struct OcrResult
{
    std::string text;
    float confidence = 0.0f;
};

// Thin wrapper around a single reused TessBaseAPI instance. Construction
// (Init) is the expensive part, so create one OcrEngine and call
// recognize() repeatedly rather than constructing per call.
class OcrEngine
{
public:
    OcrEngine();
    ~OcrEngine();

    OcrEngine(const OcrEngine &) = delete;
    OcrEngine &operator=(const OcrEngine &) = delete;

    // gray must be a single-channel 8-bit image.
    OcrResult recognize(const cv::Mat &gray);

private:
    tesseract::TessBaseAPI *api_;
};
