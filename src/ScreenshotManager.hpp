
#include <vector>
#include <memory>
#include <cstring>

#include <X11/Xutil.h>

#include "ChangedRectangle.hpp"

using namespace std;

class ScreenshotManager {

private:
    
    // Function to capture an image of a small area
    XImage* captureImage(int top, int left, int width, int height) {
        XImage* ximage = XGetImage(
            display, RootWindow(
                display, DefaultScreen(display)),
                left, top, width, height, AllPlanes, ZPixmap
            );

        if (!ximage) {
            throw runtime_error("Failed to capture image of the specified area.");
        }

        return ximage;
    }

    // Function to check if an area has changed
    bool isAreaChanged(int x, int y, XImage* currentImage) {
        if (firstCapture || !previousImages[y][x]) {
            // First iteration, store the current image for future comparisons
            previousImages[y][x] = currentImage;
            return true;
        }

        // Compare the current image with the previous one
        if (memcmp(currentImage->data, previousImages[y][x]->data, currentImage->bytes_per_line * currentImage->height) != 0) {
            // Images are different, area has changed
            XDestroyImage(previousImages[y][x]);
            previousImages[y][x] = currentImage;
            return true;
        }

        // Images are the same, area has not changed
        XDestroyImage(currentImage);
        return false;
    }

    // Member variables
    Display* display;
    int screenWidth;
    int screenHeight;
    int numImagesX;
    int numImagesY;
    int smallImageWidth;
    int smallImageHeight;

    bool firstCapture = true;

    // 2D array to store previous images for comparison
    std::vector<std::vector<XImage*>> previousImages{};

public:
    // Constructor
    ScreenshotManager(int numImagesX, int numImagesY):
        numImagesX(numImagesX),
        numImagesY(numImagesY)
    {
        
        // Initialize Xlib
        display = XOpenDisplay(nullptr);
        if (!display) {
            throw runtime_error("Unable to open X display.");
        }

        // Calculate the number of small images vertically and horizontally
        screenWidth = DisplayWidth(display, DefaultScreen(display));
        screenHeight = DisplayHeight(display, DefaultScreen(display));
        smallImageWidth = (int)((double)screenWidth / numImagesX);
        smallImageHeight = (int)((double)screenHeight / numImagesY);
        // numImagesX = screenWidth / smallImageWidth;
        // numImagesY = screenHeight / smallImageHeight;

        // Initialize the previousImages 2D array with nullptrs
        for (int y = 0; y < numImagesY; ++y) {
            vector<XImage*> row;
            for (int x = 0; x < numImagesX; ++x) {
                row.push_back(nullptr);
            }
            previousImages.push_back(row);
        }
    }

    // Destructor
    ~ScreenshotManager() {
        // Free up all allocated resources, including XImages
        // ...

        // Close the display
        XCloseDisplay(display);
    }

    std::vector<ChangedRectangle> changedRectangles;

    // Function to capture changes in the screen
    const std::vector<ChangedRectangle>& captureChanges() {
        changedRectangles.clear();

        // Iterate through each small area
        for (int y = 0; y < numImagesY; y++) {
            for (int x = 0; x < numImagesX; x++) {
                // Calculate the coordinates and size of the small area
                int top = y * smallImageHeight;
                int left = x * smallImageWidth;

                // Call a function to capture the image of the small area
                XImage* ximage = captureImage(top, left, smallImageWidth, smallImageHeight);

                // Check if the captured image is different from the previous one
                if (isAreaChanged(x, y, ximage)) {
                    // Store the changed rectangle in the vector
                    ChangedRectangle change;
                    try {
                        change.fromXImage(*ximage);
                        change.top = top;
                        change.left = left;
                        changedRectangles.push_back(change);
                    } catch (exception &e) {
                        cout << "Image capture error: " << e.what() << endl;
                    }
                }
            }
        }

        firstCapture = false;

        return changedRectangles;
    }

    int getScreenWidth() {
        return screenWidth;
        // int screen = DefaultScreen(display);
        // return DisplayWidth(display, screen);
    }

    int getScreenHeight() {
        return screenHeight;
        // int screen = DefaultScreen(display);
        // return DisplayHeight(display, screen);
    }

    vector<ChangedRectangle> getAllRectangles() {
        vector<ChangedRectangle> allRectangles;
        int x = 0, y = 0;
        for (const vector<XImage*>& previousImageRow: previousImages) {            
            for (const XImage* previousImage: previousImageRow) {
                // Calculate the coordinates and size of the small area
                int top = y * smallImageHeight;
                int left = x * smallImageWidth;

                // Store the changed rectangle in the vector
                ChangedRectangle rect;
                try {
                    rect.fromXImage(*previousImage);
                    rect.top = top;
                    rect.left = left;
                    allRectangles.push_back(rect);
                } catch (exception &e) {
                    cout << "Fullscreen image error: " << e.what() << endl;
                }
                x++;
            }
            x = 0;
            y++;
        }
        return allRectangles;
    }
};
