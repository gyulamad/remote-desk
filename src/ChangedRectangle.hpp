#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <X11/Xlib.h>

using namespace std;

class ChangedRectangle {
protected:
    struct RGB { 
        uint8_t r, g, b; 
    };
    int width, height;
    vector<RGB> pixels;

public:
    int left, top;

    string toString() const {
        stringstream ss;
        ss << left << "," << top << "," << width << "," << height;

        for (const auto& pixel : pixels) {
            ss << "," << static_cast<int>(pixel.r) << ","
               << static_cast<int>(pixel.g) << ","
               << static_cast<int>(pixel.b);
        }

        return ss.str();
    }

    void fromString(const string& serialized) {
        istringstream ss(serialized);
        char comma;
        ss >> left >> comma >> top >> comma >> width >> comma >> height;

        pixels.clear();

        int r, g, b;
        while (ss >> comma >> r >> comma >> g >> comma >> b) {
            pixels.push_back({static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)});
        }
    }

    void fromXImage(const XImage& ximage) {
        if (ximage.depth != 24) {
            throw runtime_error("Unsupported color depth. Only 24-bit color depth is supported.");
        }

        left = 0;
        top = 0;
        width = ximage.width;
        height = ximage.height;

        pixels.clear();

        for (int y = 0; y < ximage.height; ++y) {
            for (int x = 0; x < ximage.width; ++x) {
                unsigned long pixel = XGetPixel(const_cast<XImage*>(&ximage), x, y);
                RGB color;
                color.r = (pixel >> 16) & 0xFF;
                color.g = (pixel >> 8) & 0xFF;
                color.b = pixel & 0xFF;
                pixels.push_back(color);
            }
        }
    }

    void toXImage(XImage& ximage) const {
        if (ximage.depth != 24) {
            throw runtime_error("Unsupported color depth. Only 24-bit color depth is supported.");
        }

        ximage.width = width;
        ximage.height = height;

        for (int y = 0; y < ximage.height; ++y) {
            for (int x = 0; x < ximage.width; ++x) {
                unsigned long pixel = (pixels[y * width + x].r << 16) |
                                      (pixels[y * width + x].g << 8) |
                                      pixels[y * width + x].b;
                XPutPixel(&ximage, x, y, pixel);
            }
        }
    }
};
