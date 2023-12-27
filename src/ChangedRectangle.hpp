#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <X11/Xlib.h>

using namespace std;

class ChangedRectangle {
protected:
    void validatePixelsSize() const {
        if (pixels.size() != width * height)
            throw runtime_error("Incorrupted image size (" + to_string(pixels.size()) + " != " + to_string(width * height)  + ")");
    }
public:
    struct RGB { 
        uint8_t r, g, b; 
    };

    int width, height, left, top;
    vector<RGB> pixels;
    int incorrupted = 0;

    string toString() const {
        validatePixelsSize();

        stringstream ss;
        ss << left << "," << top << "," << width << "," << height;

        for (const RGB& pixel : pixels) {
            ss << "," << (int)pixel.r << "," << (int)pixel.g << "," << (int)pixel.b;
        }

        return ss.str();
    }

    void fromString(const string& serialized) {
        istringstream ss(serialized);
        char comma1, comma2, comma3;
        incorrupted = false;
        ss >> left >> comma1 >> top >> comma2 >> width >> comma3 >> height;
        if (comma1 != ',' || comma2 != ',' || comma3 != ',') {
            throw runtime_error("Incorrupted image meta");
        }

        pixels.clear();

        int r, g, b;
        while (ss >> comma1 >> r >> comma2 >> g >> comma3 >> b) {
            if (comma1 != ',' || comma2 != ',' || comma3 != ',') {
                throw runtime_error("Incorrupted image pixel");
            }
            pixels.push_back({static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)});
        }

        validatePixelsSize();
    }

    void fromXImage(const XImage& ximage) {
        if (ximage.depth != 24) {
            throw runtime_error("Converting from XImage: Unsupported color depth. Only 24-bit color depth is supported.");
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

        validatePixelsSize();
    }

    // void toXImage(XImage& ximage) const {
    //     if (ximage.depth != 24) {
    //         throw runtime_error("Converting to XImage: Unsupported color depth. Only 24-bit color depth is supported.");
    //     }

    //     // ximage.width = width;
    //     // ximage.height = height;

    //     for (int y = 0; y < height; ++y) {
    //         for (int x = 0; x < width; ++x) {
    //             RGB rgb = pixels[y * width + x];
    //             unsigned long pixel = (rgb.r << 16) |
    //                                   (rgb.g << 8) |
    //                                   rgb.b;
    //             XPutPixel(&ximage, x, y, pixel);
    //         }
    //     }
    // }
};
