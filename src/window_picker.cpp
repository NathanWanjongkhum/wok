#include "window_picker.h"

#include <X11/cursorfont.h>
#include <cstdio>

namespace
{
    Window find_toplevel(Display *display, Window root, Window start)
    {
        Window w = start;
        while (true)
        {
            Window root_ret, parent_ret, *children = nullptr;
            unsigned int nchildren = 0;
            if (!XQueryTree(display, w, &root_ret, &parent_ret, &children, &nchildren))
            {
                return w;
            }
            if (children != nullptr)
            {
                XFree(children);
            }
            if (parent_ret == root || parent_ret == None)
            {
                return w;
            }
            w = parent_ret;
        }
    }
}

Window pick_window(Display *display)
{
    Window root = DefaultRootWindow(display);
    Cursor cursor = XCreateFontCursor(display, XC_crosshair);

    int grab = XGrabPointer(display, root, False, ButtonPressMask,
                             GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);
    if (grab != GrabSuccess)
    {
        fprintf(stderr, "Failed to grab pointer for window selection\n");
        XFreeCursor(display, cursor);
        return None;
    }

    fprintf(stderr, "Click the target window...\n");

    Window target = None;
    XEvent event;
    while (target == None)
    {
        XNextEvent(display, &event);
        if (event.type == ButtonPress)
        {
            Window root_ret, child_ret;
            int rx, ry, wx, wy;
            unsigned int mask;
            XQueryPointer(display, root, &root_ret, &child_ret, &rx, &ry, &wx, &wy, &mask);
            target = (child_ret == None) ? root : find_toplevel(display, root, child_ret);
        }
    }

    XUngrabPointer(display, CurrentTime);
    XFreeCursor(display, cursor);
    return target;
}
