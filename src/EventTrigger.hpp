#pragma once

#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

class EventTrigger {
public:
    void triggerKeyEvent(KeySym key, bool press) {
        // KeyCode keyCode = XKeysymToKeycode(display, key);
        XTestFakeKeyEvent(display, key, press, CurrentTime);
        XFlush(display);
    }

    void triggerMouseEvent(unsigned int button, bool press) {   
        int x, y;
        Window root, child;
        unsigned int mask;

        // Get the root window and mouse cursor position
        XQueryPointer(display, DefaultRootWindow(display), &root, &child, &x, &y, &x, &y, &mask);

        // Trigger the mouse event at the current cursor position
        XTestFakeButtonEvent(display, button, press, CurrentTime);
        XWarpPointer(display, None, DefaultRootWindow(display), 0, 0, 0, 0, x, y);
        XFlush(display);
    }

    void triggerMouseMoveEvent(int x, int y) {
        XWarpPointer(display, None, DefaultRootWindow(display), 0, 0, 0, 0, x, y);
        XFlush(display);
    }

    // Add Linux-specific screenshot capture here

    EventTrigger() {
        display = XOpenDisplay(NULL);
        if (!display) {
            std::cerr << "Error opening display" << std::endl;
        }
    }

    ~EventTrigger() {
        if (display) {
            XCloseDisplay(display);
        }
    }

private:
    Display* display;
};