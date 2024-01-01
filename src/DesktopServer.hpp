#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>
#include <cmath>

#include "fileio.hpp"
#include "tcp.hpp"
#include "EventTrigger.hpp"
#include "Screenshot.hpp"
// #include "ScreenshotManager.hpp"

using namespace std;
using namespace chrono;

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
        { "wr", adaptWindowResize },
    };

    static void noUpdates(DesktopServer* that, int socket, const vector<int>& args) {
        // TODO
    }

    static void triggerJoin(DesktopServer* that, int socket, const vector<int>& args) {
        // TODO
    }

    static void triggerKeyPress(DesktopServer* that, int socket, const vector<int>& args) {
        that->eventTrigger.triggerKeyEvent(args.at(0), true);
    }

    static void triggerKeyRelease(DesktopServer* that, int socket, const vector<int>& args) {
        that->eventTrigger.triggerKeyEvent(args.at(0), false);
    }

    static void triggerMousePress(DesktopServer* that, int socket, const vector<int>& args) {
        that->eventTrigger.triggerMouseEvent(args.at(0), true);
    }

    static void triggerMouseRelease(DesktopServer* that, int socket, const vector<int>& args) {
        that->eventTrigger.triggerMouseEvent(args.at(0), false);    
    }

    static void triggerMouseMove(DesktopServer* that, int socket, const vector<int>& args) {
        that->eventTrigger.triggerMouseMoveEvent(args.at(0), args.at(1));
    }

    static void adaptWindowResize(DesktopServer* that, int socket, const vector<int>& args) {
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
    double captureFreq = 100;

    void recvUpdates(int socket) {
        string msg = server.recv(socket);
        cout << "socket(" << socket << "): " << msg << endl;
        if (msg.empty()) cout << "Client disconnected" << endl;
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

    unsigned char* jpeg = nullptr;
    unsigned long size = 0;

    size_t sizePrev = 0;
    double passedPrev = 0;
    double quality = 0.001;

public:

    DesktopServer(TCPServer& server): server(server) {}

    ~DesktopServer() {}

    void runEventLoop() {
        // vector<ChangedRectangle> changes;
        while (true) {

            // server polling only to accept new client connections,
            // (main poll in client side)
            while (server.poll()); 

            if (server.sockets(true).empty()) continue;

            if (size) {
                int64_t before = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

                // cout << "write: " << file_write("send.jpg", (const char*)jpeg, size) << endl;
                for (int socket: server.sockets(true)) {
                    if (!server.send_arr(socket, jpeg, size, 0)) {
                        server.disconnect(socket, "Couldn't send image");
                        continue;
                    }
                    //recvUpdates(socket);
                }

                // Align quality to transfer speed
                int64_t after = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                int64_t passed = after - before;
                if (passed > captureFreq * 1.25) { // TODO: quality parameters to constants or parameters
                    quality *= 0.8;
                    captureFreq *= 1.25;
                }
                else if (passed <= captureFreq * 0.8) {
                    quality *= 1.25;
                    captureFreq *= 0.8;
                }
                passedPrev = passed;
                if (quality > 0.8) quality = 0.8;
                if (quality < 0.1) quality = 0.1;
                if (captureFreq > 3000) captureFreq = 3000;
                if (captureFreq < 100) captureFreq = 100;

                cout << "p:" << passed << " q:" << quality << " f:" << captureFreq << endl;


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
                
                size = screenshot.captureJpeg(jpeg, quality * 100);
                
                // send only if changed (TODO: it can be more optimized, also send only the changed are, for e.g dont send fill screen for a blinking cursor or if only a small window changes)
                if (size == sizePrev) size = 0;
                if (size != 0) sizePrev = size;


                captureNextAt = now + captureFreq;
            }
        }
    }
};
