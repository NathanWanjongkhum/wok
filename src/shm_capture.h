#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <opencv2/core.hpp>

// Captures arbitrary sub-rectangles of an X11 window via the MIT-SHM
// extension into BGRA cv::Mats. One shared XImage/segment is reused across
// calls, sized for the largest region the caller will ever request.
class ShmCapture
{
public:
    ShmCapture(Display *display, int max_width, int max_height);
    ~ShmCapture();

    ShmCapture(const ShmCapture &) = delete;
    ShmCapture &operator=(const ShmCapture &) = delete;

    // Captures a w x h region at (x, y) relative to the drawable's top-left.
    // drawable may be a Window or a composite backing Pixmap (see
    // WindowRedirect) -- both are Drawables as far as XShmGetImage cares.
    // Requires w <= max_width and h <= max_height from the constructor.
    // Returns an empty Mat if the request fails (e.g. window not viewable).
    cv::Mat capture(Drawable drawable, int x, int y, int w, int h);

private:
    Display *display_;
    XShmSegmentInfo shm_info_{};
    XImage *image_ = nullptr;
    int bytes_per_pixel_ = 0;
};
