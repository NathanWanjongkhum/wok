#include "overlay_window.h"

#include <X11/extensions/shape.h>
#include <algorithm>
#include <vector>

OverlayWindow::OverlayWindow(Display *display, int x, int y, unsigned int width, unsigned int height)
    : display_(display)
{
    Window root = DefaultRootWindow(display_);
    XSetWindowAttributes attrs{};
    attrs.override_redirect = True;
    attrs.background_pixel = 0;

    window_ = XCreateWindow(display_, root, x, y, width, height, 0, CopyFromParent, InputOutput, CopyFromParent,
                             CWOverrideRedirect | CWBackPixel, &attrs);

    // Click-through: the window still occupies screen space for the shape
    // set in redraw(), but with an empty input shape every pointer event
    // passes to whatever's underneath instead of being delivered here.
    XShapeCombineRectangles(display_, window_, ShapeInput, 0, 0, nullptr, 0, ShapeSet, 0);

    gc_ = XCreateGC(display_, window_, 0, nullptr);

    // Start with an empty bounding shape (no rects_ yet) so the window is
    // fully invisible until the first set_rect() call. Without this, the
    // window's default full-rectangle shape shows solid background_pixel
    // (black) over the whole overlay area the instant it's mapped.
    redraw();

    XMapRaised(display_, window_);
    XFlush(display_);
}

OverlayWindow::~OverlayWindow()
{
    XFreeGC(display_, gc_);
    XDestroyWindow(display_, window_);
    XFlush(display_);
}

void OverlayWindow::set_rect(const std::string &id, int x, int y, int w, int h, unsigned long color, int thickness)
{
    rects_[id] = Rect{x, y, w, h, color, thickness};
    redraw();
}

void OverlayWindow::clear_rect(const std::string &id)
{
    rects_.erase(id);
    redraw();
}

void OverlayWindow::clear_all()
{
    rects_.clear();
    redraw();
}

void OverlayWindow::move(int x, int y)
{
    XMoveWindow(display_, window_, x, y);
    XFlush(display_);
}

void OverlayWindow::redraw()
{
    std::vector<XRectangle> strips;
    for (const auto &entry : rects_)
    {
        const Rect &r = entry.second;
        int t = std::max(1, r.thickness);
        strips.push_back({static_cast<short>(r.x), static_cast<short>(r.y), static_cast<unsigned short>(r.w),
                           static_cast<unsigned short>(t)});
        strips.push_back({static_cast<short>(r.x), static_cast<short>(r.y + r.h - t),
                           static_cast<unsigned short>(r.w), static_cast<unsigned short>(t)});
        strips.push_back({static_cast<short>(r.x), static_cast<short>(r.y), static_cast<unsigned short>(t),
                           static_cast<unsigned short>(r.h)});
        strips.push_back({static_cast<short>(r.x + r.w - t), static_cast<short>(r.y),
                           static_cast<unsigned short>(t), static_cast<unsigned short>(r.h)});
    }

    // The window only ever occupies these border strips -- the interior
    // isn't part of the window's visible region at all.
    XShapeCombineRectangles(display_, window_, ShapeBounding, 0, 0, strips.data(), static_cast<int>(strips.size()),
                             ShapeSet, 0);

    // Shape only carries geometry, not color, so paint each rect's strips
    // with its own color now that they're the window's whole visible area.
    for (const auto &entry : rects_)
    {
        const Rect &r = entry.second;
        int t = std::max(1, r.thickness);
        XSetForeground(display_, gc_, r.color);
        XFillRectangle(display_, window_, gc_, r.x, r.y, r.w, t);
        XFillRectangle(display_, window_, gc_, r.x, r.y + r.h - t, r.w, t);
        XFillRectangle(display_, window_, gc_, r.x, r.y, t, r.h);
        XFillRectangle(display_, window_, gc_, r.x + r.w - t, r.y, t, r.h);
    }

    XFlush(display_);
}
