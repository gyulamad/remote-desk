#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>

#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "tcp.hpp"
#include "ChangedRectangle.hpp"

using namespace std;

class DesktopClient {
private:
    TCPClient client;

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
                                      10, 10, 800, 600, 1,
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

    bool displayChangedRectangle(const ChangedRectangle& rect) {

        // Check if the specified region is within the bounds of the window
        XWindowAttributes windowAttrs;
        XGetWindowAttributes(display, window, &windowAttrs);

        // if (rect.left < 0 || rect.top < 0 ||
        //     rect.left + rect.width + windowAttrs.border_width >= windowAttrs.width ||
        //     rect.top + rect.height + windowAttrs.border_width >= windowAttrs.height) {
        //     //cout << "Specified region is outside the bounds of the window" << endl;
        //     return false;
        // }

        // Display the ChangedRectangle pixels on the window

        // std::vector<XPoint> points;
        // std::vector<unsigned long> colors;

        size_t pixsize = rect.pixels.size();
        ChangedRectangle::RGB prevColor = {0, 0, 0}, color = prevColor;
        XSetForeground(display, gc, (color.r << 16) | (color.g << 8) | color.b);
        for (short y = rect.top; y < rect.top + rect.height; ++y) {
            for (short x = rect.left; x < rect.left + rect.width; ++x) {
                int pixelIndex = (y - rect.top) * rect.width + (x - rect.left);
                if (pixsize <= pixelIndex) break;
                // const ChangedRectangle::ReducedRGB& pixelColor = rect.pixels[pixelIndex];
                // XPoint point = {x, y};
                // points.push_back(point);
                // colors.push_back((pixelColor.r << 16) | (pixelColor.g << 8) | pixelColor.b);
                ChangedRectangle::RGB color = rect.pixels[pixelIndex].toRGB();
                //putPixel(x, y, color.r, color.g, color.b);
                if (color != prevColor) {
                    XSetForeground(display, gc, (color.r << 16) | (color.g << 8) | color.b);
                    prevColor = color;
                }
                XDrawPoint(display, window, gc, x, y);
            }
        }
        // if (!points.empty()) {
        //     XDrawPoints(display, window, gc, &points[0], points.size(), CoordModeOrigin);
        //     XSetForeground(display, gc, 0);  // Reset color to avoid affecting subsequent drawing
        //     XFlush(display);
        // }

        return true;
    }

    // void putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    //     XSetForeground(display, gc, (r << 16) | (g << 8) | b);
    //     XDrawPoint(display, window, gc, x, y);
    // }

public:
    DesktopClient(const string& ipaddr, uint16_t port): 
        display(nullptr), 
        window(0)
    {
        init();
        createWindow();
        
        client.connect(ipaddr, port);
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
            while (client.poll()) {
                for (int sock: client.sockets()) {
                    size_t changes;
                    client.recv(sock, (char*)&changes, sizeof(changes), 0);
                    for (size_t i = 0; i < changes; i++) {
                        ChangedRectangle rect;
                        if (rect.recv(client, sock))
                            displayChangedRectangle(rect);
                    }
                    break; // we are connecting to only one server
                }
            }

            if (!XPending(display)) continue;
            XEvent event;
            XNextEvent(display, &event);
            while (XPending(display)) XNextEvent(display, &event);

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
