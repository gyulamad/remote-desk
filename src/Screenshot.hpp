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

    XImage* captureXImage(int left, int top, int width, int height) {
        // *** Capture screen as an XImage ***

        XImage* xImage = XGetImage(
            display, RootWindow(
                display, DefaultScreen(display)
            ),
            left, top, width, height, AllPlanes, ZPixmap
        );

        if (!xImage) {
            throw runtime_error("Failed to capture image of the specified area.");
        }

        return xImage;
    }

    void destroyXImage(XImage* xImage) {
        // *** Free up XImage ***
        XDestroyImage(xImage);
    }

    size_t xImageToJpeg(XImage* xImage, unsigned char*& jpeg, int quality = 50) {

        // Debugging output
        // std::cout << "XImage depth: " << xImage->depth << std::endl;
        // std::cout << "XImage bits per pixel: " << xImage->bits_per_pixel << std::endl;
        // std::cout << "XImage bytes per line: " << xImage->bytes_per_line << std::endl;
// exit(1);
        // *** Convert XImage to jpeg ***

        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        // Initialize the JPEG compression object
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);

        // Set in-memory destination
        unsigned long size = 0;
        jpeg_mem_dest(&cinfo, &jpeg, &size);
    

        // Set image parameters
        cinfo.image_width = xImage->width;
        cinfo.image_height = xImage->height;
        cinfo.input_components = 4; // Assuming RGB format
        cinfo.in_color_space = JCS_EXT_BGRX;
        // NOTE: these are user specific and need more tests:
        cinfo.dct_method = JDCT_FASTEST;  // or JDCT_ISLOW, JDCT_FLOAT, etc.

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE); // TODO: adjust the quality to the connection speed
        // jpeg_set_colorspace(&cinfo, JCS_GRAYSCALE);

        // Set default compression parameters
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

        return size;
    }

    unsigned long captureJpeg(unsigned char*& jpeg, int quality = 50) {

        XImage* xImage = captureXImage(0, 0, screenWidth, screenHeight);

        size_t size = xImageToJpeg(xImage, jpeg, quality);

        destroyXImage(xImage);

        return size;
    }

};
