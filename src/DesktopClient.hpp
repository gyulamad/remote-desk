#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>

#include <unistd.h>

#include <jpeglib.h>
#include <setjmp.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "tcp.hpp"
#include "fileio.hpp"
// #include "ChangedRectangle.hpp"

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
                                      10, 10, 1000, 600, 1,
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

    // vector<RGBPACK_CLASS> displayCache;
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
            // displayCache.resize(windowWidth * windowHeight);
            // for (RGBPACK_CLASS& dc: displayCache) dc.color = 0;
        }
    }


    vector<string> updates;
    void sendUpdates(int socket) {
        if (updates.empty()) return sendNoUpdate(socket);
        string update = updates[0];
        if (!client.send(socket, update)) cerr << "unable send update:" << update << endl;
        else updates.erase(updates.begin()); 
    }

    void sendNoUpdate(int socket) {
        if (!client.send(socket, "np0")) cerr << "unable send no-update" << endl;
    }

protected:

    // Assume img is the original XImage
    XImage* resizeXImage(Display* display, XImage* original, int new_width, int new_height) {
        XImage* resized = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)),
                                    original->depth, ZPixmap, 0, NULL, new_width, new_height,
                                    original->bitmap_pad, original->bytes_per_line);

        if (!resized) throw runtime_error("Failed to create XImage");
        resized->data = (char*)malloc(resized->bytes_per_line * resized->height);

        if (!resized->data) throw runtime_error("Memory allocation error");
        for (int y = 0; y < new_height; ++y)
            for (int x = 0; x < new_width; ++x)
                XPutPixel(resized, x, y, XGetPixel(
                    original, x * original->width / new_width,
                    y * original->height / new_height
                ));
            

        return resized;
    }

    unsigned char* jpeg = nullptr;
    size_t size;

    void drawJpeg() {
        const int x = 0, y = 0; // position of the image on the window

        // Check if jpeg is nullptr or size is 0
        if (!jpeg || size == 0) {
            throw runtime_error("Invalid JPEG data");
        }

        // Load the JPEG image from the vector
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        // Initialize error manager
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);


        jpeg_mem_src(&cinfo, jpeg, size);
        (void) jpeg_read_header(&cinfo, TRUE);
        (void) jpeg_start_decompress(&cinfo);

        // int width = cinfo.output_width;
        // int height = cinfo.output_height;
        // int numChannels = cinfo.output_components;


        // Boucle parcours de l'image - Image browsing loop
        unsigned char *image32 = (unsigned char *)malloc(cinfo.output_width*cinfo.output_height*4);
        unsigned char *p = image32;
        // Pour chaque ligne - for each line
        // memory to store line
        unsigned char *linebuffer = (unsigned char*)malloc(cinfo.output_width * cinfo.output_components);

        while (cinfo.output_scanline < cinfo.output_height){
            JSAMPROW buffer[1];

            buffer[0] =  linebuffer;

            jpeg_read_scanlines(&cinfo, buffer, 1); 

            // Pour chaque pixel de la ligne - for each pixel of the  line
            for(unsigned int i=0; i<cinfo.output_width; i++){
                *p++ = linebuffer[i * cinfo.output_components + 2]; // B
                *p++ = linebuffer[i * cinfo.output_components + 1]; // G
                *p++ = linebuffer[i * cinfo.output_components];     // R
                *p++ = 0;
            }
        }
        free(linebuffer);
        // printf("Lecture de l'image terminee.\n");

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        // printf("Fermeture du fichier reussie.\n");

        XImage* ximage = XCreateImage(display, DefaultVisual(display, 0), DefaultDepth(display, DefaultScreen(display)), ZPixmap, 0, (char*)image32, cinfo.output_width, cinfo.output_height, 32, 0);

        // printf("Creation de l'image reussie.\n");

        // Affichage de l'image - Shows the image
        XImage* resized = resizeXImage(display, ximage, windowWidth, windowHeight);
        XPutImage(display, window, DefaultGC(display, 0), resized, 0, 0, 0, 0, cinfo.output_width, cinfo.output_height);
        XDestroyImage(resized);
        // XFlush(display);
        // printf("Affichage de l'image reussie.\n");


        XFlush(display);
        

        free(image32);
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

        // vector<ChangedRectangle> rects;
        while (true) {

            // Check for screen changes from the server
            while (client.poll()) {
                for (int socket: client.sockets()) {

                    // size_t changes;
                    // if (-1 == client.recv(socket, (char*)&changes, sizeof(changes), 0)) {
                    //     client.disconnect(socket, "unable to recieve changes count");
                    //     break;
                    // }
                    // rects.resize(changes);
                    // for (size_t i = 0; i < changes; i++) {
                    //     // ChangedRectangle rect;
                    //     if (-1 == rects[i].recv(client, socket)) {
                    //         client.disconnect(socket, "unable to recieve change");
                    //         break;
                    //     } 
                    //     // else
                    //     //     rects[i] = rect;
                    // }
                    // update the server about our state
                    size = client.recv_arr(socket, (void**)&jpeg, 0);
                    if (!size) {
                        client.disconnect(socket, "Unable to recieve image");
                        continue;
                    }
                    // cout << "write: " << 
                    //     file_write("recv.jpg", (const char*)jpeg, size) 
                    //     << endl;                
                    // showjpg("recv.jpg");
                    drawJpeg();
                    client.free_arr((void**)&jpeg);
                    //sendUpdates(socket);
                    break; // we are connecting to only one server
                }
            }


            // // // show changes if anything left to see..
            // // for (const ChangedRectangle& rect: rects) 
            // //     displayChangedRectangle(rect);
            // // //rects.clear();
            // if (size) {
            //     cout << "write: " << file_write("recv.jpg", (const char*)jpeg, size) << endl;
            //     //drawJpeg();
            //     // free(jpeg);
            //     size = 0;
            // }


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
