#include <opencv2/opencv.hpp>
#include <cstdlib>
#include <iostream>

int main()
{
    const char *url = std::getenv("PHONE_CAM_URL");
    if (!url)
    {
        std::cerr << "Set PHONE_CAM_URL\n";
        return 1;
    }

    cv::VideoCapture cap(url);
    if (!cap.isOpened())
    {
        std::cerr << "Failed to open stream\n";
        return 1;
    }

    cv::Mat frame;
    while (cap.read(frame) && !frame.empty())
    {
        cv::imshow("Phone Camera", frame);
        if (cv::waitKey(1) == 27)
            break;
    }
    return 0;
}