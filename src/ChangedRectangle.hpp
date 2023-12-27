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

    int width, height, left, top;
    vector<RGB> pixels;
    // int incorrupted = 0;


    // Serialize the ChangedRectangle into a buffer
    vector<char> serialize() const {
        vector<char> buffer;
        buffer.resize(sizeof(int) * 4 + pixels.size() * sizeof(RGB));
        char* ptr = buffer.data();

        // Copy integer members to the buffer
        memcpy(ptr, &width, sizeof(int));
        ptr += sizeof(int);

        memcpy(ptr, &height, sizeof(int));
        ptr += sizeof(int);

        memcpy(ptr, &left, sizeof(int));
        ptr += sizeof(int);

        memcpy(ptr, &top, sizeof(int));
        ptr += sizeof(int);

        // Copy pixel data to the buffer
        memcpy(ptr, pixels.data(), pixels.size() * sizeof(RGB));

        return buffer;
    }
    
    // Deserialize a buffer into a ChangedRectangle object
    static ChangedRectangle deserialize(const std::vector<char>& buffer) {
        ChangedRectangle rect;

        // Validate the buffer size
        if (buffer.size() < sizeof(int) * 4) {
            throw std::runtime_error("Invalid buffer size for deserialization");
        }

        const char* ptr = buffer.data();

        // Copy integer members from the buffer
        memcpy(&rect.width, ptr, sizeof(int));
        ptr += sizeof(int);

        memcpy(&rect.height, ptr, sizeof(int));
        ptr += sizeof(int);

        memcpy(&rect.left, ptr, sizeof(int));
        ptr += sizeof(int);

        memcpy(&rect.top, ptr, sizeof(int));
        ptr += sizeof(int);

        // Calculate the expected size of the pixel vector
        size_t expectedSize = rect.width * rect.height * sizeof(ChangedRectangle::RGB);

        // Validate the buffer size for pixel data
        if (buffer.size() < sizeof(int) * 4 + expectedSize) {
            throw std::runtime_error("Invalid buffer size for pixel data deserialization");
        }

        // Resize the pixel vector
        rect.pixels.resize(rect.width * rect.height);

        // Copy pixel data from the buffer
        memcpy(rect.pixels.data(), ptr, expectedSize);

        // Validate the actual size of the pixel vector
        rect.validatePixelsSize();

        return rect;
    }

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
        // incorrupted = false;
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
