#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>

#include "tcp.hpp"
#include "EventTrigger.hpp"
#include "ScreenshotManager.hpp"

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

    vector<ChangedRectangle> allRects;
    vector<int> fullRefreshNeededSockets;
    static void adaptWindowResize(DesktopServer* that, int socket, const vector<int>& args) {
        that->clientWidth = args[0];
        that->clientHeight = args[1];
        that->fullRefreshNeededSockets.push_back(socket);
        if (that->allRects.empty())
            that->allRects = that->screenshotManager.getAllRectangles();
    }

    // vector<string> clientAddresses;
    // bool clientJoined = false;
    ScreenshotManager screenshotManager = ScreenshotManager(10, 6);
    int originWidth = screenshotManager.getScreenWidth();
    int originHeight = screenshotManager.getScreenHeight();
    int clientWidth = 800; // client have to send it to update.
    int clientHeight = 600; // it is now only the assumed default.
    EventTrigger eventTrigger;
    TCPServer& server;
    long long captureNextAt = 0;
    long long captureFreq = 100;

    bool resizeAndSendRectangles(int socket, const vector<ChangedRectangle>& rects) const {
        size_t size = rects.size();
        if (!size) return true;
        if (!server.send(socket, (const char*)&size, sizeof(size), 0)) {
            server.disconnect(socket, "unable to send changed rectangles count");
            return false;
        }
        for (const ChangedRectangle& rect: rects) {
            ChangedRectangle resized = rect.resize(
                originWidth, originHeight, 
                clientWidth, clientHeight // TODO: it should be a vector, each client can have different window size
            );
            if (!resized.send(server, socket)) {
                server.disconnect(socket, "partial image sending failed");
                return false;
            }
        }
        return true;
    }

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

public:

    DesktopServer(TCPServer& server): server(server) {}

    ~DesktopServer() {}

    void runEventLoop() {
        vector<ChangedRectangle> changes;
        while (true) {
            
            // server polling only to accept new client connections,
            // (main poll in client side)
            while (server.poll()); 

            if (server.sockets(true).empty()) continue;

            if (changes.size()) {
                for (int socket: server.sockets(true)) {

                    auto it = find(
                        fullRefreshNeededSockets.begin(), 
                        fullRefreshNeededSockets.end(), 
                        socket
                    );
                    if (it != fullRefreshNeededSockets.end()) {                        
                        if (!resizeAndSendRectangles(socket, allRects)) cerr << "unable to send all rectangles" << endl;
                        fullRefreshNeededSockets.erase(it);
                        if (!fullRefreshNeededSockets.size()) allRects.clear();
                        
                    }
                    else if (!resizeAndSendRectangles(socket, changes)) cerr << "unable to send changed rectangles" << endl;

                    recvUpdates(socket);
                }
                changes.clear();
            }

            long long now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            if (now >= captureNextAt) {
                // cout << "Capture screen" << endl;
                changes = screenshotManager.captureChanges();
                captureNextAt = now + captureFreq;
            }
        }
    }
};
