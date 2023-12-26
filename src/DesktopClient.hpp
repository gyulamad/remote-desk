#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "UDPClient.hpp"
#include "ChangedRectangle.hpp"

using namespace std;

class DesktopClient {
protected:

    typedef void (*EventCallback)(DesktopClient*, const ChangedRectangle&);

    const map<string, EventCallback> eventCallbacks = {
        { "cr", triggerChangedRectangle },
    };

    static void triggerChangedRectangle(DesktopClient* that, const ChangedRectangle& changedRect) {
        // show updated screen area on the client window
        cout << "triggerChangedRectangle" << endl;
        that->showChangedRectangle(changedRect);
    }

    void showChangedRectangle(const ChangedRectangle& changedRect) {
        // TODO: receive the server screen size and adjust to the window size with linear interpolation
        cout << "put image" << endl;
        if (!XPutImage(display, window, gc, changedRect.ximage, 0, 0, 
            changedRect.left, changedRect.top, 
            changedRect.ximage->width, changedRect.ximage->height)
        ) {
            char b[10000];
            cout << XGetErrorText(display, XGetErrorDatabaseText(display, "X request", "X Error", "X Resource", nullptr, 0), b, 0);
            std::cout << " - Error putting image to window" << std::endl;
        }
        cout << "image out" << endl;
    }

    UDPClient client = UDPClient("127.0.0.1", 9876);
public:
    DesktopClient(): 
        display(nullptr), 
        window(0)
    {
        init();
        createWindow();
    }

    ~DesktopClient() {
        cleanup();
    }

    void runEventLoop() {
        if (!display) {
            cerr << "Error: Display not initialized." << endl;
            return;
        }

        client.send("jn");

        while (true) {
            // if (client.isDataAvailable()) {
                UDPMessage receivedMessage = client.receive(); 
                if (receivedMessage.length > 2) {
                    EventCallback eventCallback = eventCallbacks.at(receivedMessage.data.substr(0, 2));

                    if (eventCallback == triggerChangedRectangle) {
                        size_t colonPos = receivedMessage.data.find(":");
                        string sizePart;
                        string dataPart;
                        if (colonPos != string::npos) {
                            cout << "OK colonPos != string::npos" << endl;
                            // Extract size and data from the received string
                            sizePart = receivedMessage.data.substr(2, colonPos - 2); // Adjusted bounds
                            dataPart = receivedMessage.data.substr(colonPos + 1);
                            
                            // Continue processing...
                            cout << "OK Continue processing..." << endl;
                        } else {
                            // Handle case where colon is not found in the string
                            cerr << "Colon not found in received message: " << receivedMessage.data << endl;
                        }

                        // Parse position and size information
                        stringstream ss(sizePart);
                        ChangedRectangle receivedRect;
                        int width, height;
                        char comma;
                        ss >> receivedRect.left >> comma >> receivedRect.top >> comma 
                            >> width >> comma >> height;    
                        cout << "create image.." << endl;                
                        receivedRect.ximage = XCreateImage(XOpenDisplay(nullptr), CopyFromParent, 32, ZPixmap, 0, const_cast<char*>(dataPart.c_str()), width, height, 32, width * 4);
                        triggerChangedRectangle(this, receivedRect);
                        //cout << "destroy image starts" << endl;
                        // XDestroyImage(receivedRect.ximage);
                        //cout << "image destroyed" << endl;
                    }
                }
            // }

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

private:
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
        client.send(("kp" + to_string(keyEvent.keycode)).c_str());
    }

    void handleKeyRelease(const XKeyEvent& keyEvent) {
        // Stub method for key release event handling
        cout << "Key released: " << keyEvent.keycode << endl;
        client.send(("kr" + to_string(keyEvent.keycode)).c_str());
    }

    void handleMousePress(const XButtonEvent& buttonEvent) {
        // Stub method for mouse button press event handling
        cout << "Mouse button pressed: " << buttonEvent.button << endl;
        client.send(("mp" + to_string(buttonEvent.button)).c_str());
    }

    void handleMouseRelease(const XButtonEvent& buttonEvent) {
        // Stub method for mouse button release event handling
        cout << "Mouse button released: " << buttonEvent.button << endl;
        client.send(("mr" + to_string(buttonEvent.button)).c_str());
    }

    void handleMouseMove(const XMotionEvent& motionEvent) {
        // Stub method for mouse move event handling
        cout << "Mouse moved: (" << motionEvent.x << ", " << motionEvent.y << ")" << endl;
        client.send(("mm" + to_string(motionEvent.x) + "," + to_string(motionEvent.y)).c_str());
    }

    void handleResize(const XConfigureEvent& configureEvent) {
        // Stub method for window resize event handling
        cout << "Window resized: " << configureEvent.width << " x " << configureEvent.height << endl;
        client.send(("wr" + to_string(configureEvent.width) + "," + to_string(configureEvent.height)).c_str());
    }
};
