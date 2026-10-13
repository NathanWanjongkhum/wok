#pragma once
#include <X11/Xlib.h>

// Uses the X Composite extension to keep the target window fully rendered to
// an off-screen pixmap even when it's partially or fully covered on screen.
// Plain XShmGetImage/XGetImage on a Window only sees what's currently
// visible, so occluded regions read back as garbage without this.
//
// Uses CompositeRedirectAutomatic: the server keeps painting the window to
// the screen normally (nothing changes visually) while also maintaining the
// backing pixmap we read from. No separate compositor process required.
class WindowRedirect
{
public:
    WindowRedirect(Display *display, Window window);
    ~WindowRedirect();

    WindowRedirect(const WindowRedirect &) = delete;
    WindowRedirect &operator=(const WindowRedirect &) = delete;

    // Returns the current backing pixmap. Cheap to call every tick --
    // re-fetches so a resize of the target window is always picked up.
    Pixmap pixmap();

private:
    Display *display_;
    Window window_;
    Pixmap pixmap_ = 0;
};
