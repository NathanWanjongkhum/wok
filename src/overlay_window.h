#pragma once
#include <X11/Xlib.h>
#include <string>
#include <unordered_map>

// A borderless, click-through window that draws rectangle outlines using
// the X Shape extension instead of alpha transparency -- the window's
// visible region is carved down to just the border strips, so this works
// on plain X11 with no compositor (picom, etc.) required.
class OverlayWindow
{
public:
    // x, y, width, height place the overlay in screen coordinates -- e.g.
    // the target window's own screen position and size, so rect coordinates
    // passed to set_rect() line up directly with region coordinates.
    OverlayWindow(Display *display, int x, int y, unsigned int width, unsigned int height);
    ~OverlayWindow();

    OverlayWindow(const OverlayWindow &) = delete;
    OverlayWindow &operator=(const OverlayWindow &) = delete;

    // Adds or replaces a labeled rectangle, in coordinates relative to the
    // overlay's own origin. color is a packed 0xRRGGBB pixel value.
    void set_rect(const std::string &id, int x, int y, int w, int h, unsigned long color, int thickness = 2);
    void clear_rect(const std::string &id);
    void clear_all();

    // Repositions the overlay in screen coordinates. Rects stay defined
    // relative to the overlay's own origin, so nothing else needs updating
    // when the target window (and this overlay along with it) moves.
    void move(int x, int y);

private:
    struct Rect
    {
        int x, y, w, h;
        unsigned long color;
        int thickness;
    };

    void redraw();

    Display *display_;
    Window window_;
    GC gc_;
    std::unordered_map<std::string, Rect> rects_;
};
