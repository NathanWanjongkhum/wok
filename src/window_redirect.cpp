#include "window_redirect.h"

#include <X11/extensions/Xcomposite.h>
#include <stdexcept>

WindowRedirect::WindowRedirect(Display *display, Window window) : display_(display), window_(window)
{
    int event_base, error_base;
    if (!XCompositeQueryExtension(display_, &event_base, &error_base))
    {
        throw std::runtime_error("X server does not support the Composite extension");
    }

    XCompositeRedirectWindow(display_, window_, CompositeRedirectAutomatic);
}

WindowRedirect::~WindowRedirect()
{
    if (pixmap_ != 0)
    {
        XFreePixmap(display_, pixmap_);
    }
    XCompositeUnredirectWindow(display_, window_, CompositeRedirectAutomatic);
}

Pixmap WindowRedirect::pixmap()
{
    if (pixmap_ != 0)
    {
        XFreePixmap(display_, pixmap_);
    }
    pixmap_ = XCompositeNameWindowPixmap(display_, window_);
    return pixmap_;
}
