#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>

#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <asio.hpp>
#include "ChangedRectangle.hpp"

using namespace std;

class DesktopClient {
private:
    asio::io_context ioContext;
    asio::ip::tcp::socket socket;

    Display* display;
    Window window;
    GC gc;  // Graphics context
    Atom wmDeleteMessage;

    void init() {
        display = XOpenDisplay(NULL);
        if (!display) {
            cerr << "Error: Unable to open display." << endl;
            exit(EXIT_FAILURE);
        }

        wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display, DefaultRootWindow(display), &wmDeleteMessage, 1);
    }

    void createWindow() {
        int screen = DefaultScreen(display);

        window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                      10, 10, 400, 300, 1,
                                      BlackPixel(display, screen),
                                      WhitePixel(display, screen));

        XSelectInput(display, window, StructureNotifyMask | KeyPressMask | KeyReleaseMask |
                                       ButtonPressMask | ButtonReleaseMask | PointerMotionMask);

        XMapWindow(display, window);

        // Set window title (optional)
        XStoreName(display, window, "Your Window Title");

        // Create the GC and store it
        gc = DefaultGC(display, screen);
        
        // Flush pending events
        XFlush(display);
    }

    void showImage(int top, int left, XImage* ximage) {
        XPutImage(display, window, gc, ximage, 0, 0, left, top, ximage->width, ximage->height);
        XFlush(display);
    }

    void cleanup() {
        if (display) {
            XDestroyWindow(display, window);
            XCloseDisplay(display);
        }
    }

    void handleKeyPress(const XKeyEvent& keyEvent) {
        // Stub method for key press event handling
        cout << "Key pressed: " << keyEvent.keycode << endl;
    }

    void handleKeyRelease(const XKeyEvent& keyEvent) {
        // Stub method for key release event handling
        cout << "Key released: " << keyEvent.keycode << endl;
    }

    void handleMousePress(const XButtonEvent& buttonEvent) {
        // Stub method for mouse button press event handling
        cout << "Mouse button pressed: " << buttonEvent.button << endl;
    }

    void handleMouseRelease(const XButtonEvent& buttonEvent) {
        // Stub method for mouse button release event handling
        cout << "Mouse button released: " << buttonEvent.button << endl;
    }

    void handleMouseMove(const XMotionEvent& motionEvent) {
        // Stub method for mouse move event handling
        cout << "Mouse moved: (" << motionEvent.x << ", " << motionEvent.y << ")" << endl;
    }

    void handleResize(const XConfigureEvent& configureEvent) {
        // Stub method for window resize event handling
        cout << "Window resized: " << configureEvent.width << " x " << configureEvent.height << endl;
    }

protected:


    bool hasData() {
        return socket.available() > 0;
    }

    void receiveChangedRectangle(ChangedRectangle& rect) {
        // Read the buffer size asynchronously
        size_t bufferSize;
        asio::async_read(socket, asio::buffer(&bufferSize, sizeof(size_t)),
            [this, &rect, bufferSize](std::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Read the buffer data asynchronously
                    std::vector<char> buffer(bufferSize);
                    asio::async_read(socket, asio::buffer(buffer),
                        [this, &rect, buffer](std::error_code ec, std::size_t /*length*/) {
                            if (!ec) {
                                // Deserialize the buffer into ChangedRectangle
                                rect = ChangedRectangle::deserialize(buffer);
                                
                                // Call the displayChangedRectangle method in DesktopClient
                                displayChangedRectangle(rect);

                                // Continue processing the received data
                                // (e.g., additional logic if needed)
                            } else {
                                std::cerr << "Error reading buffer data: " << ec.message() << std::endl;
                            }
                        });
                } else {
                    std::cerr << "Error reading buffer size: " << ec.message() << std::endl;
                }
            });
    }

    bool displayChangedRectangle(const ChangedRectangle& rect) {
        cout << "put image" << endl;

        // Check if the specified region is within the bounds of the window
        XWindowAttributes windowAttrs;
        XGetWindowAttributes(display, window, &windowAttrs);

        if (rect.left < 0 || rect.top < 0 ||
            rect.left + rect.width + windowAttrs.border_width * 2 + 10 >= windowAttrs.width ||
            rect.top + rect.height + windowAttrs.border_width * 2 + 10 >= windowAttrs.height) {
            cout << "Specified region is outside the bounds of the window" << endl;
            return false;
        }

        // Display the ChangedRectangle pixels on the window
        for (int y = rect.top; y < rect.top + rect.height; ++y) {
            for (int x = rect.left; x < rect.left + rect.width; ++x) {
                int pixelIndex = (y - rect.top) * rect.width + (x - rect.left);
                if (rect.pixels.size() <= pixelIndex) break;
                const ChangedRectangle::RGB& pixelColor = rect.pixels[pixelIndex];
                putPixel(x, y, pixelColor.r, pixelColor.g, pixelColor.b);
            }
        }

        cout << "image out" << endl;
        return true;
    }

    void putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        XSetForeground(display, gc, (r << 16) | (g << 8) | b);
        XDrawPoint(display, window, gc, x, y);
        XFlush(display);
    }

public:
    DesktopClient(const string& ipaddr, unsigned short port): 
        socket(ioContext),
        display(nullptr), 
        window(0)
    {
        init();
        createWindow();

        asio::ip::tcp::resolver resolver(ioContext);
        asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(ipaddr, std::to_string(port));
        asio::connect(socket, endpoints);
    }

    ~DesktopClient() {
        cleanup();
    }

    void runEventLoop() {
        if (!display) {
            cerr << "Error: Display not initialized." << endl;
            return;
        }

        while (true) {
            // Check for screen changes from the server
            if (hasData()) {
                ChangedRectangle rect;
                receiveChangedRectangle(rect);

                // Display the received ChangedRectangle on the window
                // displayChangedRectangle(rect);
            }

            if (!XPending(display)) continue;
            XEvent event;
            XNextEvent(display, &event);

            switch (event.type) {
                case KeyPress:
                    handleKeyPress(event.xkey);
                    break;

                case KeyRelease:
                    handleKeyRelease(event.xkey);
                    break;

                case ButtonPress:
                    handleMousePress(event.xbutton);
                    break;

                case ButtonRelease:
                    handleMouseRelease(event.xbutton);
                    break;

                case MotionNotify:
                    handleMouseMove(event.xmotion);
                    break;

                case ConfigureNotify:
                    handleResize(event.xconfigure);
                    break;

                case Expose:
                    // Handle expose events (redraw if needed)
                    break;

                case ClientMessage:
                    if (static_cast<unsigned long>(event.xclient.data.l[0]) == wmDeleteMessage)
                        return;
                    break;

                default:
                    // Handle other events if needed
                    break;
            }
        }
    }

};
