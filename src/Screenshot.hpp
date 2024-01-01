#pragma once

#include <stdexcept>
#include <vector>

#include <jpeglib.h>
#include <X11/Xutil.h>

using namespace std;

class Screenshot {
protected:

    Display* display;
    int screenWidth;
    int screenHeight;

public:

    Screenshot() {

        // Initialize Xlib
        display = XOpenDisplay(nullptr);
        if (!display) {
            throw runtime_error("Unable to open X display.");
        }

        screenWidth = DisplayWidth(display, DefaultScreen(display));
        screenHeight = DisplayHeight(display, DefaultScreen(display));
    }

    virtual ~Screenshot() {
        // Close the display
        XCloseDisplay(display);
    }

    unsigned long captureJpeg(unsigned char*& jpeg) {

        // *** Capture screen as an XImage ***

        XImage* xImage = XGetImage(
            display, RootWindow(
                display, DefaultScreen(display)),
                0, 0, screenWidth, screenHeight, AllPlanes, ZPixmap
            );

        if (!xImage) {
            throw runtime_error("Failed to capture image of the specified area.");
        }

        // *** Convert XImage to jpeg ***

        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        // Initialize the JPEG compression object
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);

        // Set in-memory destination
        unsigned long size;
        jpeg_mem_dest(&cinfo, &jpeg, &size);

        // Set image parameters
        cinfo.image_width = xImage->width;
        cinfo.image_height = xImage->height;
        cinfo.input_components = 3; // Assuming RGB format
        cinfo.in_color_space = JCS_RGB;

        // Set default compression parameters
        jpeg_set_defaults(&cinfo);
        jpeg_start_compress(&cinfo, TRUE);

        // Write image data
        JSAMPROW row_pointer;
        for (int y = 0; y < xImage->height; ++y) {
            row_pointer = (JSAMPROW)(&xImage->data[y * xImage->bytes_per_line]);
            jpeg_write_scanlines(&cinfo, &row_pointer, 1);
        }

        // Finish compression
        jpeg_finish_compress(&cinfo);

        // Clean up
        jpeg_destroy_compress(&cinfo);

        // *** Free up XImage ***
        XDestroyImage(xImage);

        return size;
    }

};
