#pragma once

#include <X11/Xlib.h>

// Define a struct for the changed rectangles
struct ChangedRectangle {
    int top, left;
    XImage* ximage;  // Use XImage for storing screen data
};