#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>
#include <cmath>

#include "../libs/clib/clib/files.hpp"
#include "../libs/clib/clib/tcp.hpp"
#include "../libs/clib/clib/chiper.hpp"
#include "../libs/clib/clib/rand.hpp"
#include "EventTrigger.hpp"
#include "Screenshot.hpp"
#include "Rectangle.hpp"
// #include "ScreenshotManager.hpp"

using namespace std;
using namespace chrono;
using namespace clib;

class DesktopServer {
protected:

    typedef void (*EventCallback)(DesktopServer*, int, const vector<int>&);

    const map<string, EventCallback> eventCallbacks = {
        { "np", noUpdates },
        { "jn", triggerJoin },
        { "kp", triggerKeyPress },
        { "kr", triggerKeyRelease },
        { "mp", triggerMousePress },
        { "mr", triggerMouseRelease },
        { "mm", triggerMouseMove },
        { "wr", adaptWindowResize }, // YAGNI?
    };

    static void noUpdates(DesktopServer* /* that */, int /* socket */, const vector<int>& /* args */) {
        // TODO
    }

    static void triggerJoin(DesktopServer* /* that */, int /* socket */, const vector<int>& /* args */) {
        // TODO
    }

    static void triggerKeyPress(DesktopServer* that, int /* socket */, const vector<int>& args) {
        that->eventTrigger.triggerKeyEvent((unsigned)args.at(0), true);
    }

    static void triggerKeyRelease(DesktopServer* that, int /* socket */, const vector<int>& args) {
        that->eventTrigger.triggerKeyEvent((unsigned)args.at(0), false);
    }

    static void triggerMousePress(DesktopServer* that, int /* socket */, const vector<int>& args) {
        that->eventTrigger.triggerMouseEvent((unsigned)args.at(0), /*args.at(0), args.at(1),*/ true);
    }

    static void triggerMouseRelease(DesktopServer* that, int /* socket */, const vector<int>& args) {
        that->eventTrigger.triggerMouseEvent((unsigned)args.at(0), /*args.at(0), args.at(1),*/ false);    
    }

    static void triggerMouseMove(DesktopServer* that, int /* socket */, const vector<int>& args) {
        that->eventTrigger.triggerMouseMoveEvent(args.at(0), args.at(1));
    }

    static void adaptWindowResize(DesktopServer* /* that */, int /* socket */, const vector<int>& /* args */) {
        // that->clientWidth = args[0];
        // that->clientHeight = args[1];
        // that->fullRefreshNeededSockets.push_back(socket);
        // if (that->allRects.empty())
        //     that->allRects = that->screenshotManager.getAllRectangles();
    }

    Screenshot screenshot;
    EventTrigger eventTrigger;
    TCPServer& server;
    long long captureNextAt = 0;
    const int captureFreqMax = 500;
    const int captureFreqMin = 100;
    double captureFreq = captureFreqMin;

    void recvUpdates(int socket) {
        string msg = server.recv(socket);
        // cout << "socket(" << socket << "): " << msg << endl;
        if (msg.empty()) cout << "Client disconnected" << endl;
        vector<string> updates = str_split("\n", msg);
        for (const string& msg: updates) {
            if (msg.size() > 3) {
                cout << "Received: " << msg << endl;
                
                vector<int> eventArgs;
                stringstream ss(msg.substr(2, msg.size()));
                string token;
                while (getline(ss, token, ','))
                    eventArgs.push_back(stoi(token));

                eventCallbacks.at(msg.substr(0, 2))(this, socket, eventArgs);
                
            }
        }
    }

    // // TODO: lib function
    // vector<string> str_split(const string& separator, const string& data) {
    //     if (data.empty()) return {};

    //     vector<string> tokens;
    //     size_t start = 0, end = 0;

    //     while ((end = data.find(separator, start)) != string::npos) {
    //         tokens.push_back(data.substr(start, end - start));
    //         start = end + separator.length();
    //     }

    //     // Add the last token (or the only token if no separator found)
    //     tokens.push_back(data.substr(start));

    //     return tokens;
    // }

    bool onClientConnect(int newSocket) {
        cout << "Recieve cliid..." << endl;
        const string cliid = server.recv(newSocket);
        const string pubkey_fname = "pubkeys/" + cliid + ".pubkey.pem";
        if (!file_exists(pubkey_fname))
            return !server.disconnect(newSocket, "Public key not found");
        
        // if (now() - file_get_mtime(pubkey_fname) > pubkey_empiry)
        //     return !server.disconnect(newSocket, "Public key expired");

        string challenge = rands(100);
        string encrypted = encrypt(challenge, pubkey_fname);
        cout << "Sending challenge..." << endl;
        if (!server.send(newSocket, encrypted))
            return !server.disconnect(newSocket, "Failed to send auth challenge");
        cout << "Recieving solution..." << endl;
        if (challenge != server.recv(newSocket)) 
            return !server.disconnect(newSocket, "Auth challenge failed");

        cout << "Client connected: " << newSocket << endl;
        return true;
    }

    unsigned char* jpeg = nullptr;
    unsigned long size = 0;

    size_t sizePrev = 0;
    double passedPrev = 0;

    double qualityHigh = 0.8;
    double qualityLow = 0.001;
    double quality = qualityHigh;

    XImage* refXImage = nullptr;
    XImage* actXImage = nullptr;
    Rectangle rect;

public:

    DesktopServer(TCPServer& server): server(server) {
        refXImage = screenshot.captureXImage();
        actXImage = screenshot.captureXImage();

    }

    ~DesktopServer() {
        if (refXImage) screenshot.destroyXImage(refXImage);
        if (actXImage) screenshot.destroyXImage(actXImage);
    }

    void runEventLoop() {
        // vector<ChangedRectangle> changes;
        while (true) {

            // server polling only to accept new client connections,
            // (main poll in client side)
            int newSocket = 0;
            while (server.poll(newSocket))
                if (newSocket)
                    onClientConnect(newSocket);

            if (server.sockets(true).empty()) continue;

            if (size) { // TODO: if no changes on the screen still send a "no-image" image so that the client still can send there updates about there user inputs such as mouse and keyboard events
                int64_t before = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

                // cout << "write: " << file_write("send.jpg", (const char*)jpeg, size) << endl;
                for (int socket: server.sockets(true)) {
                    // TODO: also send the mouse cursor if changed
                    rect.width = screenshot.getScreenWidth();
                    rect.height = screenshot.getScreenHeight();
                    // cout << "send:" << size 
                    //     << " rect:" 
                    //     << rect.left << "," << rect.top << " " 
                    //     << rect.width << "," << rect.height << " - " 
                    //     << sizeof(rect) 
                    //     << endl;
                    if (!server.send_arr(socket, &rect, sizeof(rect), 0)) {
                        server.disconnect(socket, "Couldn't send image meta");
                        continue;
                    }
                    if (!server.send_arr(socket, jpeg, size, 0)) {
                        server.disconnect(socket, "Couldn't send image");
                        continue;
                    }
                    recvUpdates(socket);
                }

                // Align quality to transfer speed
                int64_t after = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                int64_t passed = after - before;
                if ((double)passed > captureFreq * 1.25) { // TODO: quality parameters to constants or parameters
                    quality *= 0.8;
                    captureFreq *= 1.25;
                }
                else if ((double)passed <= captureFreq * 0.8) {
                    quality *= 1.25;
                    captureFreq *= 0.8;
                }
                passedPrev = (double)passed;
                if (quality > qualityHigh) quality = qualityHigh;
                if (quality < qualityLow) quality = qualityLow;
                if (captureFreq > captureFreqMax) captureFreq = captureFreqMax;
                if (captureFreq < captureFreqMin) captureFreq = captureFreqMin;

                // cout << "p:" << passed << " q:" << quality << " f:" << captureFreq << endl;


                // free(jpeg);
                size = 0;
            }

            // if (changes.size()) {
            //     for (int socket: server.sockets(true)) {

            //         auto it = find(
            //             fullRefreshNeededSockets.begin(), 
            //             fullRefreshNeededSockets.end(), 
            //             socket
            //         );
            //         if (it != fullRefreshNeededSockets.end()) {                        
            //             if (!resizeAndSendRectangles(socket, allRects)) cerr << "unable to send all rectangles" << endl;
            //             fullRefreshNeededSockets.erase(it);
            //             if (!fullRefreshNeededSockets.size()) allRects.clear();
                        
            //         }
            //         else if (!resizeAndSendRectangles(socket, changes)) cerr << "unable to send changed rectangles" << endl;

            //         recvUpdates(socket);
            //     }
            //     changes.clear();
            // }

            long long now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();            
            if (now >= captureNextAt) {

                bool fast = false;
                if (quality < qualityHigh * 0.8 || captureFreq > captureFreqMin * 1.25) {
                    cout << "SLOW!!" << endl;

                    actXImage = screenshot.captureXImage();
                    rect = compareXImages(refXImage, actXImage, fast);
                    screenshot.destroyXImage(refXImage);
                    refXImage = actXImage;
                    actXImage = nullptr;

                    size = 0;
                    if (rect.width > 0 && rect.height > 0) {
                        size = screenshot.captureJpeg(
                            rect.left, rect.top, 
                            rect.width, rect.height, 
                            jpeg, (int)(quality * 100)
                        );
                    }
                } else {
                    fast = true;

                    actXImage = screenshot.captureXImage();
                    rect = compareXImages(refXImage, actXImage, fast);
                    screenshot.destroyXImage(refXImage);
                    refXImage = actXImage;
                    actXImage = nullptr;

                    size = 0;
                    if (fast) {
                        rect.top = 0;
                        rect.left = 0;
                        rect.width = screenshot.getScreenWidth();
                        rect.height = screenshot.getScreenHeight();
                        size = screenshot.captureJpeg(
                            jpeg, (int)(quality * 100)
                        );
                    }
                }

                
                // size = screenshot.captureJpeg(jpeg, quality * 100);
                
                // // send only if changed (TODO: it can be more optimized, also send only the changed are, for e.g dont send fill screen for a blinking cursor or if only a small window changes)
                // if (size == sizePrev) size = 0;
                // if (size != 0) sizePrev = size;


                captureNextAt = now + (long long)captureFreq;
            }
        }
    }

protected:
    Rectangle compareXImages(XImage* xImage1, XImage* xImage2, bool& fast) {
        Rectangle bounds = { -1, -1, 0, 0 }; // Initialize bounds with invalid values

        int minX = xImage1->width;  // Set to maximum possible value
        int minY = xImage1->height; // Set to maximum possible value
        int maxX = -1;  // Set to minimum possible value
        int maxY = -1;  // Set to minimum possible value

        for (int y = 0; y < xImage1->height; ++y) {
            for (int x = 0; x < xImage1->width; ++x) {
                unsigned long pixel1 = XGetPixel(xImage1, x, y);
                unsigned long pixel2 = XGetPixel(xImage2, x, y);

                if (pixel1 != pixel2) {
                    // Pixels are different, update bounding rectangle
                    if (fast) {
                        fast = true;
                        return bounds;
                    }
                    minX = std::min(minX, x);
                    minY = std::min(minY, y);
                    maxX = std::max(maxX, x);
                    maxY = std::max(maxY, y);
                }
            }
        }

        // Check if any differences were found
        if (minX <= maxX && minY <= maxY) {
            bounds.left = minX;
            bounds.top = minY;
            bounds.width = maxX - minX + 1;
            bounds.height = maxY - minY + 1;
        }

        return bounds;
    }

};
