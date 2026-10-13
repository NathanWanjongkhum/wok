#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>

#include "overlay_window.h"
#include "window_picker.h"

namespace
{
    volatile std::sig_atomic_t g_stop = 0;

    void handle_sigint(int)
    {
        g_stop = 1;
    }

    constexpr unsigned long kDraggingColor = 0xffff00;  // yellow
    constexpr unsigned long kCommittedColor = 0x00ff00; // green
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <output_regions_file> [window_id_hex]\n", argv[0]);
        fprintf(stderr, "Drag a box over each piece of text you want to track.\n");
        fprintf(stderr, "Escape cancels the box you're dragging (or quits, if not dragging).\n");
        fprintf(stderr, "Enter saves and quits. Regions are auto-tagged region_1, region_2, ...\n");
        fprintf(stderr, "-- rename the tags in the output file afterward if you want.\n");
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

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(display, target, &attrs))
    {
        fprintf(stderr, "Failed to query target window geometry\n");
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }

    Window root = DefaultRootWindow(display);
    Window child_unused;
    int screen_x = 0, screen_y = 0;
    XTranslateCoordinates(display, target, root, 0, 0, &screen_x, &screen_y, &child_unused);

    // Report ConfigureNotify (move/resize) for the target window so the
    // overlay can follow it if the user drags it around mid-calibration --
    // otherwise screen_x/screen_y above go stale and the border stops
    // lining up with the text.
    XSelectInput(display, target, StructureNotifyMask);

    std::ofstream out(argv[1], std::ios::app);
    if (!out)
    {
        fprintf(stderr, "Failed to open output file: %s\n", argv[1]);
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }

    // OverlayWindow and the grabs below all live in this nested block so
    // they're torn down (via destructor / explicit ungrab) before
    // XCloseDisplay runs after the block. C++ only destructs main()'s
    // locals when main() itself returns, not wherever XCloseDisplay sits
    // textually -- without this scoping, overlay's destructor would run
    // after the display was already closed and crash.
    int region_count = 0;
    {
        OverlayWindow overlay(display, screen_x, screen_y, static_cast<unsigned int>(attrs.width),
                               static_cast<unsigned int>(attrs.height));

        Cursor cursor = XCreateFontCursor(display, XC_crosshair);
        if (XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                          GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime) != GrabSuccess)
        {
            fprintf(stderr, "Failed to grab pointer\n");
            XFreeCursor(display, cursor);
            XCloseDisplay(display);
            return EXIT_FAILURE;
        }

        // Grabbing the keyboard too is what makes Escape/Enter actually
        // work: without it, key presses go to whatever window last had
        // input focus (often the target window itself, since we just
        // clicked it), not to this process -- and since the pointer grab
        // above also blocks clicking elsewhere to refocus a terminal,
        // there'd be no way to get a keystroke to this program at all.
        if (XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess)
        {
            fprintf(stderr, "Failed to grab keyboard\n");
            XUngrabPointer(display, CurrentTime);
            XFreeCursor(display, cursor);
            XCloseDisplay(display);
            return EXIT_FAILURE;
        }

        std::signal(SIGINT, handle_sigint);
        fprintf(stderr, "Drag a box over each piece of text you want to track.\n");
        fprintf(stderr, "Escape cancels the current drag (or quits, if not dragging). Enter saves and quits.\n");
        fprintf(stderr, "Regions are saved to %s as you go, auto-tagged region_1, region_2, ...\n", argv[1]);

        bool dragging = false;
        int start_x = 0, start_y = 0;

        while (!g_stop)
        {
            if (!XPending(display))
            {
                // Poll rather than block on XNextEvent so SIGINT is noticed
                // promptly even if no X events ever arrive.
                struct timespec ts
                {
                    0, 20 * 1000 * 1000
                };
                nanosleep(&ts, nullptr);
                continue;
            }

            XEvent event;
            XNextEvent(display, &event);

            if (event.type == KeyPress)
            {
                KeySym key = XLookupKeysym(&event.xkey, 0);
                if (key == XK_Escape)
                {
                    if (dragging)
                    {
                        dragging = false;
                        overlay.clear_rect("__dragging__");
                    }
                    else
                    {
                        g_stop = 1;
                    }
                }
                else if (key == XK_Return)
                {
                    if (dragging)
                    {
                        dragging = false;
                        overlay.clear_rect("__dragging__");
                    }
                    g_stop = 1;
                }
                continue;
            }

            if (event.type == ConfigureNotify && event.xconfigure.window == target)
            {
                // Re-resolve through the full parent chain rather than
                // trusting event.xconfigure.x/y, which are relative to the
                // window's immediate parent (its WM frame), not root.
                Window child_unused2;
                XTranslateCoordinates(display, target, root, 0, 0, &screen_x, &screen_y, &child_unused2);
                overlay.move(screen_x, screen_y);
                continue;
            }

            int px, py;
            if (event.type == ButtonPress || event.type == ButtonRelease)
            {
                px = event.xbutton.x_root - screen_x;
                py = event.xbutton.y_root - screen_y;
            }
            else if (event.type == MotionNotify)
            {
                px = event.xmotion.x_root - screen_x;
                py = event.xmotion.y_root - screen_y;
            }
            else
            {
                continue;
            }

            if (event.type == ButtonPress)
            {
                dragging = true;
                start_x = px;
                start_y = py;
            }
            else if (event.type == MotionNotify && dragging)
            {
                int rx = std::min(start_x, px);
                int ry = std::min(start_y, py);
                int rw = std::abs(px - start_x);
                int rh = std::abs(py - start_y);
                overlay.set_rect("__dragging__", rx, ry, rw, rh, kDraggingColor);
            }
            else if (event.type == ButtonRelease && dragging)
            {
                dragging = false;
                overlay.clear_rect("__dragging__");

                int rx = std::min(start_x, px);
                int ry = std::min(start_y, py);
                int rw = std::abs(px - start_x);
                int rh = std::abs(py - start_y);
                if (rw < 4 || rh < 4)
                {
                    continue; // accidental click, not a real drag
                }

                std::string tag = "region_" + std::to_string(region_count + 1);
                out << tag << " " << rx << " " << ry << " " << rw << " " << rh << "\n";
                out.flush();
                ++region_count;

                fprintf(stderr, "%s: %d,%d %dx%d\n", tag.c_str(), rx, ry, rw, rh);
                overlay.set_rect(tag, rx, ry, rw, rh, kCommittedColor);
            }
        }

        XUngrabKeyboard(display, CurrentTime);
        XUngrabPointer(display, CurrentTime);
        XFreeCursor(display, cursor);
    } // overlay destructs here, while display is still open

    XCloseDisplay(display);
    fprintf(stderr, "Saved %d region(s) to %s\n", region_count, argv[1]);
    return EXIT_SUCCESS;
}
