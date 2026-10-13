#pragma once
#include <X11/Xlib.h>

// Prompts the user to click a window on screen and returns its top-level
// window id (the direct child of root), or None if the pointer grab fails.
Window pick_window(Display *display);
