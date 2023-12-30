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

    int windowWidth = 0, windowHeight = 0;
    void handleResize(const XConfigureEvent& configureEvent) {
        // Stub method for window resize event handling
        cout << "Window resized: " << configureEvent.width << " x " << configureEvent.height << endl;
        if (windowWidth != configureEvent.width || 
            windowHeight != configureEvent.height
        ) { 
            windowWidth = configureEvent.width;
            windowHeight = configureEvent.height;
            updates.push_back("wr" 
                + to_string(configureEvent.width) + "," 
                + to_string(configureEvent.height)
            );
            
        }
    }


    vector<string> updates;
    void sendUpdates() {
        if (updates.empty()) return;
        string update = updates[0];
        bool ok = false;
        for (int sock: client.sockets(true)) {
            ok = client.send(sock, update);
            break; // we are connecting only one server
        }
        if (ok) updates.erase(updates.begin());
        else cerr << "unable send update:" << update << endl;
    }

protected:

    bool displayChangedRectangle(const ChangedRectangle& rect) {

        size_t pixsize = rect.pixels.size();
        ChangedRectangle::RGB prevColor = {0, 0, 0}, color = prevColor;
        XSetForeground(display, gc, (color.r << 16) | (color.g << 8) | color.b);
        short ymax = rect.top + rect.height;
        short xmax = rect.left + rect.width;
        for (short y = rect.top; y < ymax; ++y)
            for (short x = rect.left; x < xmax; ++x) {
                int pixelIndex = (y - rect.top) * rect.width + (x - rect.left);
                if (pixsize <= pixelIndex) break;
                ChangedRectangle::RGB color = rect.pixels[pixelIndex].toRGB();
                if (color != prevColor) {
                    XSetForeground(display, gc, (color.r << 16) | (color.g << 8) | color.b);
                    prevColor = color;
                }
                XDrawPoint(display, window, gc, x, y);
            }

        return true;
    }

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

        vector<ChangedRectangle> rects;
        while (true) {

            // Check for screen changes from the server
            while (client.poll()) {
                for (int sock: client.sockets()) {
                    size_t changes;
                    if (-1 == client.recv(sock, (char*)&changes, sizeof(changes), 0)) {
                        client.disconnect(sock, "unable to recieve changes count");
                        break;
                    }
                    rects.resize(changes);
                    for (size_t i = 0; i < changes; i++) {
                        ChangedRectangle rect;
                        if (-1 == rect.recv(client, sock)) {
                            client.disconnect(sock, "unable to recieve change");
                            break;
                        } else rects[i] = rect;
                    }
                    break; // we are connecting to only one server
                }
            }

            // update the server about our state
            sendUpdates();

            // show changes if anything left to see..
            for (const ChangedRectangle& rect: rects) displayChangedRectangle(rect);
            rects.clear();


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
