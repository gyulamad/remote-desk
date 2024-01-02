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
#include "Rectangle.hpp"

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
                                      10, 10, windowWidth, windowHeight, 1,
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
        updates.push_back("kp" 
            + to_string(keyEvent.keycode)
        );
    }

    void handleKeyRelease(const XKeyEvent& keyEvent) {
        // Stub method for key release event handling
        cout << "Key released: " << keyEvent.keycode << endl;
        updates.push_back("kr" 
            + to_string(keyEvent.keycode)
        );
    }

    void handleMousePress(const XButtonEvent& buttonEvent) {
        // rect->width/hight => original screen width/height
        //      eventx / realx = windowWidth / screenWidth
        //      realx / eventx = screenWidth / windowWidth
        //      realx = (screenWidth / windowWidth) * eventx
        int x = (int)( ((double)screenWidth / (double)windowWidth) * (double)buttonEvent.x );
        int y = (int)( ((double)screenHeight / (double)windowHeight) * (double)buttonEvent.y );
        cout << "Mouse button pressed: " << buttonEvent.button << endl;
        updates.push_back("mp" 
            + to_string(buttonEvent.button) + "," 
            + to_string(x) + "," 
            + to_string(y)
        );
    }

    void handleMouseRelease(const XButtonEvent& buttonEvent) {
        // rect->width/hight => original screen width/height
        //      eventx / realx = windowWidth / screenWidth
        //      realx / eventx = screenWidth / windowWidth
        //      realx = (screenWidth / windowWidth) * eventx
        int x = (int)( ((double)screenWidth / (double)windowWidth) * (double)buttonEvent.x );
        int y = (int)( ((double)screenHeight / (double)windowHeight) * (double)buttonEvent.y );
        cout << "Mouse button released: " << buttonEvent.button << endl;
        updates.push_back("mr" 
            + to_string(buttonEvent.button) + "," 
            + to_string(x) + "," 
            + to_string(y)
        );
    }

    void handleMouseMove(const XMotionEvent& motionEvent) {
        // rect->width/hight => original screen width/height
        //      eventx / realx = windowWidth / screenWidth
        //      realx / eventx = screenWidth / windowWidth
        //      realx = (screenWidth / windowWidth) * eventx
        int x = (int)( ((double)screenWidth / (double)windowWidth) * (double)motionEvent.x );
        int y = (int)( ((double)screenHeight / (double)windowHeight) * (double)motionEvent.y );
        cout << "Mouse moved: (" << motionEvent.x << ", " << motionEvent.y << ")" << endl;
        string update = "mm" 
            + to_string(x) + "," 
            + to_string(y);
        string lst = "";
        for (const string& updt: updates)
            if (updt.substr(0, 2) == "mm") lst = updt;
        if (update != lst) updates.push_back(update);
    }

    // vector<RGBPACK_CLASS> displayCache;
    int windowWidth = 1000, windowHeight = 600; // TODO: from parameters
    void handleResize(const XConfigureEvent& configureEvent) { // YAGNI?
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
        string msg = str_concat(updates, "\n");
        if (!client.send(socket, msg)) cerr << "unable send updates" << endl;
        updates.clear();
    }

    void sendNoUpdate(int socket) {
        if (!client.send(socket, "np0")) cerr << "unable send no-update" << endl;
    }
    
    // TODO: lib function
    string str_concat(const vector<string>& strs, const string& sep) {
        if (strs.empty()) return "";
        string result = strs[0];
        for (size_t i = 1; i < strs.size(); ++i) result += sep + strs[i];
        return result;
    }

protected:

    // Assume img is the original XImage
    XImage* resizeXImage(Display* display, XImage* original, int new_width, int new_height) {
        if (new_width <= 0 || new_height <= 0) {
            throw std::invalid_argument("Invalid dimensions for resizing");
        }

        XImage* resized = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)),
                                    original->depth, ZPixmap, 0, NULL, new_width, new_height,
                                    original->bitmap_pad, 0);  // Set bytes_per_line to 0

        if (!resized) {
            throw std::runtime_error("Failed to create XImage");
        }

        resized->data = (char*)malloc(resized->bytes_per_line * resized->height);
        if (!resized->data) {
            throw std::runtime_error("Memory allocation error");
        }

        for (int y = 0; y < new_height; ++y) {
            for (int x = 0; x < new_width; ++x) {
                int oX = x * original->width / new_width;
                int oY = y * original->height / new_height;
                // if (oX < original->width && oY < original->height) {
                    XPutPixel(resized, x, y, XGetPixel(original, oX, oY));
                // }
            }
        }

        return resized;
    }
    void freeXImage(XImage* image) {
        if (image) {
            if (image->data) {
                free(image->data);
                image->data = nullptr;  // Optional: Set to nullptr after freeing to avoid double free
            }
            XDestroyImage(image);
        }
    }

    unsigned char* jpeg = nullptr;
    size_t size;
    Rectangle* rect = nullptr;
    int screenWidth = 0;
    int screenHeight = 0;

    void drawJpeg() {
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

        XImage* ximage = XCreateImage(display, DefaultVisual(display, 0), 
            DefaultDepth(display, DefaultScreen(display)), 
            ZPixmap, 0, (char*)image32, cinfo.output_width, cinfo.output_height, 32, 0);

        
        
        // ximage->width / new_width = screenWidth / windowWidth
        // new_width / ximage->width = windowWidth / screenWidth
        int new_width = (int)( ((double)windowWidth / (double)rect->width) * (double)ximage->width );
        int new_height = (int)( ((double)windowHeight / (double)rect->height) * (double)ximage->height );
        if (new_width > 0 && new_height > 0) {
            
            XImage* resized = resizeXImage(display, ximage, new_width, new_height);

            // sw/ww = ox/x
            // ww/sw = x/ox
            // (ww*ox) / sw = x; 
            
            // rect->width/hight => original screen width/height
            int x = (int)( ((double)windowWidth * (double)rect->left) / (double)rect->width );
            int y = (int)( ((double)windowHeight * (double)rect->top) / (double)rect->height );
                

            XPutImage(display, window, DefaultGC(display, 0),
                resized, 0, 0, x, y, cinfo.output_width, cinfo.output_height);
            freeXImage(resized);
        }
        // XDestroyImage(resized);


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
                    if (!client.recv_arr(socket, (void**)&rect, 0)) {
                        client.disconnect(socket, "Unable to recieve image meta");
                        continue;
                    }
                    screenWidth = rect->width;
                    screenHeight = rect->height;
                    size = client.recv_arr(socket, (void**)&jpeg, 0);
                    if (!size) {
                        client.disconnect(socket, "Unable to recieve image");
                        continue;
                    }
                    // cout << "write: " << 
                    //     file_write("recv.jpg", (const char*)jpeg, size) 
                    //     << endl;                
                    // showjpg("recv.jpg");

                    // cout << "recv:" << size << endl;
                    drawJpeg();
                    client.free_arr((void**)&rect);
                    client.free_arr((void**)&jpeg);

                    sendUpdates(socket);
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
            // while (XPending(display)) XNextEvent(display, &event);

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
