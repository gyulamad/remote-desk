#pragma once

#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

class ScreenshotManager {
public:
    ScreenshotManager() : img1(nullptr), img2(nullptr) {
        display = XOpenDisplay(NULL);
        if (!display) {
            std::cerr << "Error opening display" << std::endl;
        }
    }

    ~ScreenshotManager() {
        if (img1) XDestroyImage(img1);
        if (img2) XDestroyImage(img2);
        if (display) XCloseDisplay(display);
    }

    XImage* captureChanges() {
        // If this is the first capture, or if img1 does not exist, capture the initial image and return
        if (!img1) {
            img1 = capture();
            return img1;
        }

        // Capture a new screenshot
        img2 = capture();

        // Compare and find the bounding rectangle of differences
        int x, y, width, height;
        findBoundingRect(img1, img2, x, y, width, height);

        // Destroy the outdated image
        XDestroyImage(img1);

        // Store the current image for the next comparison
        img1 = img2;

        // Return an XImage representing the changes
        return cutImage(img2, x, y, width, height);
    }

private:
    Display* display;
    XImage* img1;  // Previous screenshot
    XImage* img2;  // Current screenshot

    XImage* capture() {
        Window root = DefaultRootWindow(display);
        return XGetImage(display, root, 0, 0, WidthOfScreen(DefaultScreenOfDisplay(display)),
                         HeightOfScreen(DefaultScreenOfDisplay(display)), AllPlanes, ZPixmap);
    }

    void findBoundingRect(XImage* img1, XImage* img2, int& x, int& y, int& width, int& height) {
        // Iterate over pixels and find the bounding rectangle of differences
        x = y = width = height = 0;

        for (int yy = 0; yy < img1->height; ++yy) {
            for (int xx = 0; xx < img1->width; ++xx) {
                unsigned long pixel1 = XGetPixel(img1, xx, yy);
                unsigned long pixel2 = XGetPixel(img2, xx, yy);

                if (pixel1 != pixel2) {
                    if (x == 0 && y == 0) {
                        // First differing pixel, set initial coordinates
                        x = xx;
                        y = yy;
                        width = height = 1;
                    } else {
                        // Expand bounding rectangle
                        x = std::min(x, xx);
                        y = std::min(y, yy);
                        width = std::max(width, xx - x + 1);
                        height = std::max(height, yy - y + 1);
                    }
                }
            }
        }
    }

    XImage* cutImage(XImage* img, int x, int y, int width, int height) {
        // Create a new XImage representing the changes
        XImage* cutImg = XCreateImage(display, CopyFromParent, img->depth, ZPixmap, 0,
                                      reinterpret_cast<char*>(img->data) + y * img->bytes_per_line + x * img->bits_per_pixel / 8,
                                      width, height, img->bitmap_pad, img->bytes_per_line);

        return cutImg;
    }
};
