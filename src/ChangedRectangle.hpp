#pragma once

#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "tcp.hpp"

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

        // Assignment operator
        RGB& operator=(const RGB& other) {
            if (this != &other) {
                r = other.r;
                g = other.g;
                b = other.b;
            }
            return *this;
        }

        // Equality operator
        bool operator==(const RGB& other) const {
            return (r == other.r) && (g == other.g) && (b == other.b);
        }

        // Inequality operator
        bool operator!=(const RGB& other) const {
            return !(*this == other);
        }
    };
    struct RGB565 {
        uint16_t color = 0;

        RGB565() {}

        RGB565(uint8_t r, uint8_t g, uint8_t b) {
            color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        }

        RGB toRGB() const {
            RGB rgb;
            rgb.r = ((color >> 11) & 0x1F) << 3;
            rgb.g = ((color >> 5) & 0x3F) << 2;
            rgb.b = (color & 0x1F) << 3;
            return rgb;
        }
    };
    struct RGB323 {
        uint8_t color;

        RGB323() {}

        RGB323(uint8_t r, uint8_t g, uint8_t b) {
            color = ((r >> 5) << 5) | ((g >> 6) << 3) | (b >> 5);
        }

        RGB toRGB() const {
            RGB rgb;
            rgb.r = ((color >> 5) & 0x07) << 5;
            rgb.g = ((color >> 3) & 0x03) << 6;
            rgb.b = (color & 0x07) << 5;
            return rgb;
        }
    };
    struct RGBMono {
        uint8_t color;

        RGBMono() {}

        RGBMono(uint8_t r, uint8_t g, uint8_t b) {
            color = (r + g + b) / 3;
        }

        RGB toRGB() const {
            RGB rgb;
            rgb.r = color;
            rgb.g = color;
            rgb.b = color;
            return rgb;
        }
    };
    struct RGB111S {
        uint8_t color;

        RGB111S() {}

        RGB111S(uint8_t r, uint8_t g, uint8_t b) {
            uint8_t s = (r + g + b) / 3;
            uint8_t f = 2;
            uint8_t r_ = (r > g * f && r > b * f) ? 0x80 : 0;
            uint8_t g_ = (g > r * f && g > b * f) ? 0x40 : 0;
            uint8_t b_ = (b > r * f && b > g * f) ? 0x20 : 0;
            s = (s >> 3);
            color = r_ | g_ | b_ | s;
        }

        RGB toRGB() const {
            RGB rgb;
            uint8_t s = (color & 0x1f) * 8;
            uint8_t f = 4;
            rgb.r = s + ((color & 0x80) ? 0xA0 : 0x0A) / f;
            rgb.g = s + ((color & 0x40) ? 0xA0 : 0x0A) / f;
            rgb.b = s + ((color & 0x20) ? 0xA0 : 0x0A) / f;
            return rgb;
        }
    };

#define ReducedRGB RGB111S

    int width, height, left, top;
    vector<ReducedRGB> pixels;

    ChangedRectangle resize(int originWidth, int originHeight, int clientWidth, int clientHeight) const {
        ChangedRectangle resized;

        double scale = clientWidth > clientHeight
            ? (double)clientWidth / originWidth
            : (double)clientHeight / originHeight; // TODO: portrait?
            
        // Calculate the new dimensions
        resized.width = (int)(width * scale) + 1;
        resized.height = (int)(height * scale) + 1;

        // Center the resized image in the client window
        resized.left = (int)(left * scale);
        resized.top = (int)(top * scale);

         // Resize the pixels count
        resized.pixels.resize(resized.width * resized.height);
        
        for (int x = 0; x < resized.width; x++) {
            for (int y = 0; y < resized.height; y++) {
                int origX = (int)((double)(x) / scale);
                int origY = (int)((double)(y) / scale);

                resized.pixels[x + y * resized.width] = pixels[origX + origY * width];
            }
        }

        return resized;
    }

    bool send(TCPSocket& tcp, int sock) const {
        vector<int> pos = { width, height, left, top };
        return 
            tcp.send_vector<int>(sock, pos) &&
            tcp.send_vector<ReducedRGB>(sock, pixels);
    }

    bool recv(TCPSocket& tcp, int sock) {
        vector<int> pos = tcp.recv_vector<int>(sock);
        if (pos.size() != 4) return false; //throw runtime_error("Invalid rectangle positions: " + to_string(pos.size()));
        width = pos.at(0);
        height = pos.at(1);
        left = pos.at(2);
        top = pos.at(3);
        pixels = tcp.recv_vector<ReducedRGB>(sock);
        return pixels.size() == width * height;
    }

    void fromXImage(const XImage& ximage) {
        if (ximage.depth != 24) {
            throw runtime_error("Converting from XImage: Unsupported color depth. Only 24-bit color depth is supported.");
        }

        left = 0;
        top = 0;
        width = ximage.width;
        height = ximage.height;

        pixels.resize(width * height);

        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                unsigned long pixel = XGetPixel(const_cast<XImage*>(&ximage), x, y);
                RGB color;
                color.r = (pixel >> 16) & 0xFF;
                color.g = (pixel >> 8) & 0xFF;
                color.b = pixel & 0xFF;
                // Convert to ReducedRGB and store in the vector
                ReducedRGB reducedColor(color.r, color.g, color.b);
                pixels[x + y * width] = reducedColor;
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
