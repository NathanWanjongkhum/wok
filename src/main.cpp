#include <X11/Xlib.h>
#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <opencv2/imgproc.hpp>
#include <thread>
#include <unordered_map>

#include "logger.h"
#include "ocr_engine.h"
#include "overlay_window.h"
#include "regions.h"
#include "shm_capture.h"
#include "window_picker.h"
#include "window_redirect.h"

namespace
{
    volatile std::sig_atomic_t g_stop = 0;

    void handle_sigint(int)
    {
        g_stop = 1;
    }

    bool region_changed(const cv::Mat &current, const cv::Mat &previous)
    {
        return previous.empty() || previous.size() != current.size() ||
               cv::countNonZero(current != previous) != 0;
    }

    constexpr unsigned long kRegionColor = 0x00ff00; // green
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <regions_file> [window_id_hex]\n", argv[0]);
        fprintf(stderr, "  regions_file: lines of \"tag x y w h\" relative to the target window\n");
        fprintf(stderr, "  window_id_hex: skip the click-to-select prompt (e.g. 0x3400007)\n");
        return EXIT_FAILURE;
    }

    std::vector<TextRegion> regions;
    try
    {
        regions = load_regions(argv[1]);
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "%s\n", e.what());
        return EXIT_FAILURE;
    }

    Display *display = XOpenDisplay(nullptr);
    if (display == nullptr)
    {
        fprintf(stderr, "Failed to open X display\n");
        return EXIT_FAILURE;
    }

    Window target = (argc >= 3) ? static_cast<Window>(strtoul(argv[2], nullptr, 0)) : pick_window(display);
    if (target == None)
    {
        fprintf(stderr, "No target window selected\n");
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }

    int max_w = 0, max_h = 0;
    for (const auto &r : regions)
    {
        max_w = std::max(max_w, r.w);
        max_h = std::max(max_h, r.h);
    }

    XWindowAttributes target_attrs;
    if (!XGetWindowAttributes(display, target, &target_attrs))
    {
        fprintf(stderr, "Failed to query target window geometry\n");
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }

    Window root = DefaultRootWindow(display);
    Window child_unused;
    int screen_x = 0, screen_y = 0;
    XTranslateCoordinates(display, target, root, 0, 0, &screen_x, &screen_y, &child_unused);

    // Report move/resize so the overlay can be kept lined up with the
    // target window while tracking runs.
    XSelectInput(display, target, StructureNotifyMask);

    try
    {
        WindowRedirect redirect(display, target);
        ShmCapture capture(display, max_w, max_h);
        OcrEngine ocr;
        CsvLogger logger("log.csv");
        std::unordered_map<std::string, cv::Mat> previous;

        OverlayWindow overlay(display, screen_x, screen_y, static_cast<unsigned int>(target_attrs.width),
                               static_cast<unsigned int>(target_attrs.height));
        for (const auto &region : regions)
        {
            overlay.set_rect(region.tag, region.x, region.y, region.w, region.h, kRegionColor);
        }

        std::signal(SIGINT, handle_sigint);
        fprintf(stderr, "Tracking %zu region(s) on window 0x%lx. Logging to log.csv. Ctrl+C to stop.\n",
                regions.size(), target);

        const auto tick = std::chrono::milliseconds(16);
        auto next_tick = std::chrono::steady_clock::now();

        while (!g_stop)
        {
            while (XPending(display))
            {
                XEvent event;
                XNextEvent(display, &event);
                if (event.type == ConfigureNotify && event.xconfigure.window == target)
                {
                    // Re-resolve through the full parent chain rather than
                    // trusting event.xconfigure.x/y, which are relative to
                    // the window's immediate parent (its WM frame), not
                    // root.
                    Window child_unused2;
                    XTranslateCoordinates(display, target, root, 0, 0, &screen_x, &screen_y, &child_unused2);
                    overlay.move(screen_x, screen_y);
                }
            }

            Pixmap frame_pixmap = redirect.pixmap();

            for (const auto &region : regions)
            {
                cv::Mat frame = capture.capture(frame_pixmap, region.x, region.y, region.w, region.h);
                if (frame.empty())
                {
                    continue;
                }

                cv::Mat gray;
                cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);

                if (region_changed(gray, previous[region.tag]))
                {
                    OcrResult result = ocr.recognize(gray);
                    fprintf(stderr, "[%s] %s (conf=%.0f)\n", region.tag.c_str(), result.text.c_str(),
                            result.confidence);
                    logger.log(region.tag, result.text, result.confidence);
                    previous[region.tag] = gray;
                }
            }

            next_tick += tick;
            std::this_thread::sleep_until(next_tick);
        }
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "%s\n", e.what());
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }

    XCloseDisplay(display);
    fprintf(stderr, "Stopped.\n");
    return EXIT_SUCCESS;
}
