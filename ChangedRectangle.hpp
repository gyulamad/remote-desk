#pragma once

#include <X11/Xlib.h>

// Define a struct for the changed rectangles
struct ChangedRectangle {
    int top, left, width, height;
    XImage* ximage;  // Use XImage for storing screen data
};