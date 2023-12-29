#pragma once

#include <iostream>
#include <sstream>
#include <cstring>
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

    int width, height, left, top;
    vector<RGB565> pixels;
    // int incorrupted = 0;

    bool send(TCPSocket& tcp, int sock) const {
        vector<int> pos = { width, height, left, top };
        return 
            tcp.send_vector<int>(sock, pos) &&
            tcp.send_vector<RGB565>(sock, pixels);
    }

    bool recv(TCPSocket& tcp, int sock) {
        vector<int> pos = tcp.recv_vector<int>(sock);
        if (pos.size() != 4) return false; //throw runtime_error("Invalid rectangle positions: " + to_string(pos.size()));
        width = pos.at(0);
        height = pos.at(1);
        left = pos.at(2);
        top = pos.at(3);
        pixels = tcp.recv_vector<RGB565>(sock);
        return pixels.size() == width * height;
    }

    // string toString() const {
    //     validatePixelsSize();

    //     stringstream ss;
    //     ss << left << "," << top << "," << width << "," << height;

    //     for (const RGB& pixel : pixels) {
    //         ss << "," << (int)pixel.r << "," << (int)pixel.g << "," << (int)pixel.b;
    //     }

    //     return ss.str();
    // }

    // void fromString(const string& serialized) {
    //     istringstream ss(serialized);
    //     char comma1, comma2, comma3;
    //     // incorrupted = false;
    //     ss >> left >> comma1 >> top >> comma2 >> width >> comma3 >> height;
    //     if (comma1 != ',' || comma2 != ',' || comma3 != ',') {
    //         throw runtime_error("Incorrupted image meta");
    //     }

    //     pixels.clear();

    //     int r, g, b;
    //     while (ss >> comma1 >> r >> comma2 >> g >> comma3 >> b) {
    //         if (comma1 != ',' || comma2 != ',' || comma3 != ',') {
    //             throw runtime_error("Incorrupted image pixel");
    //         }
    //         pixels.push_back({static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)});
    //     }

    //     validatePixelsSize();
    // }

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
                // Convert to RGB565 and store in the vector
                RGB565 color565(color.r, color.g, color.b);
                pixels.push_back(color565);
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
