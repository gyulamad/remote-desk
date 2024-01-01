// #include "src/UDPClient.hpp"
// #include "src/UDPServer.hpp"
// #include "src/AsioTCP.hpp"
#include "src/EventTrigger.hpp"
// #include "src/ScreenshotManager.hpp"
#include "src/DesktopClient.hpp"
// #include "src/AsioCommunicator.hpp"
#include "src/tcp.hpp"
// #include "src/ChangedRectangle.hpp"

#include    <stdio.h>
#include    <stdlib.h>
#include    <jpeglib.h>
#include    <X11/Xlib.h>
#include    <X11/Xutil.h>

#include "src/fileio.hpp"


int window() {
    cout << "window()....." << endl;
    Display *display = XOpenDisplay(NULL);
XEvent event;

if (display == NULL) {
    fprintf(stderr, "[ERREUR] Impossible de créer la fenêtre.\n");
    return -1;
}

int screen = DefaultScreen(display);
Window root = RootWindow(display, screen);

Window window = XCreateSimpleWindow(
    display, root, 0, 0,
    800, 600, 1,
    BlackPixel(display, screen),
    WhitePixel(display, screen));

printf("Creation de la fenetre reussie.\n");

XMapWindow(display, window);

GC gc = DefaultGC(display, screen);
XSetForeground(display, gc, BlackPixel(display, screen));
XFillRectangle(display, window, gc, 10, 10, 100, 100);
XFlush(display);

cout << "wait before draw" << endl;
sleep(3);
cout << "draw" << endl;
XSetForeground(display, gc, BlackPixel(display, screen));
XFillRectangle(display, window, gc, 10, 10, 100, 100);
XFlush(display);
cout << "drown" << endl;
sleep(3);

XFlush(display);
printf("Affichage de l'image reussie.\n");

// You might want to add a timer or condition here to exit the loop after a certain time

cout << "loooop" << endl;
while (XNextEvent(display, &event) == 0) {
    // Handle events or leave it empty if you just want to wait
    // XFlush(display); // Not necessary to flush in the event loop
}

// Your cleanup code
XUnmapWindow(display, window);
XDestroyWindow(display, window);
XCloseDisplay(display);

printf("Fermeture de la fenetre reussie.\n");

cout << "window() ends...." << endl;
return 0;
}


void test_jpeg_form_data() {
    const char* jpegfile = "recv.jpg";
    size_t fsize = file_size(jpegfile);
    char* jpeg = (char*)malloc(fsize); 
    size_t size = file_read(jpegfile, jpeg, fsize);

    // Ouverture du fichier JPEG - Opens the JPEG file
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;


    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, (const unsigned char*)jpeg, size);
    (void) jpeg_read_header(&cinfo, TRUE);
    (void) jpeg_start_decompress(&cinfo);

    printf("Largeur : %d\n", cinfo.output_width);
    printf("Hauteur : %d\n", cinfo.output_height);
    printf("Nombre de composantes : %d\n", cinfo.output_components);

    printf("Lecture de l'image en cours...\n");

    // Création de la fenêtre - Creates the window
    Display *display = XOpenDisplay(NULL);
    XEvent event;
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    if (display == NULL){
        fprintf(stderr, "[ERREUR] Impossible de créer la fenêtre.\n");
        exit(1);
    }

    Window window = XCreateSimpleWindow(
        display, root, 0, 0, 
        800, 600, 1,
                                      BlackPixel(display, screen),
                                      WhitePixel(display, screen));

    printf("Creation de la fenetre reussie.\n");

    XMapWindow(display, window);

    GC gc = DefaultGC(display, screen);
    XFlush(display);
    sleep(1);

    XSetForeground(display, gc, BlackPixel(display, screen));
    XFillRectangle(display, window, gc, 10, 10, 100, 100);
    XFlush(display);
    sleep(1);

    XSetForeground(display, gc, BlackPixel(display, screen));
    XFillRectangle(display, window, gc, 10, 10, 100, 100);
    XFlush(display);
    sleep(1);


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
    printf("Lecture de l'image terminee.\n");

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    printf("Fermeture du fichier reussie.\n");

    XImage* ximage = XCreateImage(display, DefaultVisual(display, 0), DefaultDepth(display, DefaultScreen(display)), ZPixmap, 0, (char*)image32, cinfo.output_width, cinfo.output_height, 32, 0);

    printf("Creation de l'image reussie.\n");

    // Affichage de l'image - Shows the image
   XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, cinfo.output_height, cinfo.output_width);
    
    XFlush(display);
    printf("Affichage de l'image reussie.\n");

    while(XNextEvent(display, &event) == 0){

    XFlush(display);

    free(image32);
    //
    free(jpeg);
    }

    sleep(3);

    // XDestroyImage(ximage);
    XUnmapWindow(display, window);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    printf("Fermeture de la fenetre reussie.\n");


}

int main() {

    try {
        // window();
        // ----------- jpeg from data -------------------
        test_jpeg_form_data();
        return 0;
        // ----------- jpeg -------------------

    // Ouverture du fichier JPEG - Opens the JPEG file
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;


    FILE* imgFile = fopen("recv.jpg", "rb");
    if(imgFile == NULL){
        fprintf(stderr, "[ERREUR] Impossible d'ouvrir le fichier \"%s\".\n", "[img]");
        return -1;
    }


    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, imgFile);
    (void) jpeg_read_header(&cinfo, TRUE);
    (void) jpeg_start_decompress(&cinfo);

    printf("Largeur : %d\n", cinfo.output_width);
    printf("Hauteur : %d\n", cinfo.output_height);
    printf("Nombre de composantes : %d\n", cinfo.output_components);

    printf("Lecture de l'image en cours...\n");

    // Création de la fenêtre - Creates the window
    Display *display = XOpenDisplay(NULL);
    XEvent event;
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    if (display == NULL){
        fprintf(stderr, "[ERREUR] Impossible de créer la fenêtre.\n");
        return -1;
    }

    Window window = XCreateSimpleWindow(
        display, root, 0, 0, 
        800, 600, 1,
                                      BlackPixel(display, screen),
                                      WhitePixel(display, screen));

    printf("Creation de la fenetre reussie.\n");

    XMapWindow(display, window);

    GC gc = DefaultGC(display, screen);
    XFlush(display);
    sleep(1);

    XSetForeground(display, gc, BlackPixel(display, screen));
    XFillRectangle(display, window, gc, 10, 10, 100, 100);
    XFlush(display);
    sleep(1);

    XSetForeground(display, gc, BlackPixel(display, screen));
    XFillRectangle(display, window, gc, 10, 10, 100, 100);
    XFlush(display);
    sleep(1);


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
    printf("Lecture de l'image terminee.\n");

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(imgFile);

    printf("Fermeture du fichier reussie.\n");

    XImage* ximage = XCreateImage(display, DefaultVisual(display, 0), DefaultDepth(display, DefaultScreen(display)), ZPixmap, 0, (char*)image32, cinfo.output_width, cinfo.output_height, 32, 0);

    printf("Creation de l'image reussie.\n");

    // Affichage de l'image - Shows the image
   XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, cinfo.output_height, cinfo.output_width);
    
    XFlush(display);
    printf("Affichage de l'image reussie.\n");

    while(XNextEvent(display, &event) == 0){

    XFlush(display);
    }

    sleep(3);

    // XDestroyImage(ximage);
    XUnmapWindow(display, window);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    printf("Fermeture de la fenetre reussie.\n");

    return 0;



        // ---------- RGB packing --------------

        // RGB565 rpixel1, rpixel2;
        // rpixel1 = rpixel2;

        // vector<RGB565> pixels1;
        // vector<RGB565> pixels2;
        // pixels2.push_back(RGB565());
        // pixels2.push_back(RGB565());
        // pixels1 = pixels2;
        

        // ---------------- tcp -----------------

        // --------------- ScreenshotManager -------------------

        // ScreenshotManager screenshotManager(100, 100);

        // // Example: Capture and display the result of the first capture
        // vector<ChangedRectangle> firstCapture = screenshotManager.captureChanges();

        // // ... (process the 'firstCapture' image as needed)

        // // Example: Capture changes and process the resulting image
        // vector<ChangedRectangle> changes = screenshotManager.captureChanges();

        // // ... (process the 'changes' image as needed)


        // --------- EventTrigger ----------

        EventTrigger eventTrigger;

        // Example: Trigger 'A' key press
        eventTrigger.triggerKeyEvent('A', true);

        // Example: Trigger 'A' key release
        eventTrigger.triggerKeyEvent('A', false);
        
        // Example: Trigger left mouse button press at the current cursor position
        eventTrigger.triggerMouseEvent(Button1, true);

        // Example: Trigger left mouse button release at the current cursor position
        eventTrigger.triggerMouseEvent(Button1, false);
        
        // Example: Move the mouse to coordinates (200, 200)
        eventTrigger.triggerMouseMoveEvent(200, 200);

        // ---------- UDP -----------

        // UDPServer server(9877);
        // UDPClient client("127.0.0.1", 9877);

        // // Simple example: send a message from client to server
        // client.send("Hello, Server!");


        // // Server receives the message
        // UDPMessage receivedMessage = server.receive();
        // std::cout << "Server received from client: " << receivedMessage.data << std::endl;

        // // Server sends a response to the client
        // server.send("Hello, Client!", (sockaddr*)&receivedMessage.senderAddress);

        // UDPMessage receivedMessage2 = client.receive();
        // std::cout << "Server received from server: " << receivedMessage2.data << std::endl;

        // receivedMessage = client.receive();
        // cout << receivedMessage.length << endl;

        // --------------- DesktopClient -------------------

        // AsioTCP comm;
        // DesktopClient desktopClient(comm);
        // desktopClient.runEventLoop();
    } catch (exception &e) {
        cout << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
