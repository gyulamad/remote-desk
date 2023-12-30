#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>

#include "tcp.hpp"
#include "EventTrigger.hpp"
#include "ScreenshotManager.hpp"

using namespace std;
using namespace chrono;

class DesktopServer {
protected:

    typedef void (*EventCallback)(DesktopServer*, const vector<int>&);

    const map<string, EventCallback> eventCallbacks = {
        { "jn", triggerJoin },
        { "kp", triggerKeyPress },
        { "kr", triggerKeyRelease },
        { "mp", triggerMousePress },
        { "mr", triggerMouseRelease },
        { "mm", triggerMouseMove },
        { "wr", adaptWindowResize }, // TODO: may YAGNI now but we may want to send less pixel if the client window is small
    };

    static void triggerJoin(DesktopServer* that, const vector<int>& args) {
        // TODO
    }

    static void triggerKeyPress(DesktopServer* that, const vector<int>& args) {
        that->eventTrigger.triggerKeyEvent(args.at(0), true);
    }

    static void triggerKeyRelease(DesktopServer* that, const vector<int>& args) {
        that->eventTrigger.triggerKeyEvent(args.at(0), false);
    }

    static void triggerMousePress(DesktopServer* that, const vector<int>& args) {
        that->eventTrigger.triggerMouseEvent(args.at(0), true);
    }

    static void triggerMouseRelease(DesktopServer* that, const vector<int>& args) {
        that->eventTrigger.triggerMouseEvent(args.at(0), false);    
    }

    static void triggerMouseMove(DesktopServer* that, const vector<int>& args) {
        that->eventTrigger.triggerMouseMoveEvent(args.at(0), args.at(1));
    }

    static void adaptWindowResize(DesktopServer* that, const vector<int>& args) {
        that->clientWidth = args[0];
        that->clientHeight = args[1];
    }

    // vector<string> clientAddresses;
    // bool clientJoined = false;
    ScreenshotManager screenshotManager = ScreenshotManager(4, 4);
    int originWidth = screenshotManager.getScreenWidth();
    int originHeight = screenshotManager.getScreenHeight();
    int clientWidth = 800; // TODO: client have to send it..
    int clientHeight = 600; // ...it is now only the assumed default.
    EventTrigger eventTrigger;
    TCPServer& server;
    long long captureNextAt = 0;
    long long captureFreq = 100;
public:

    DesktopServer(TCPServer& server): server(server) {}

    ~DesktopServer() {}

    void runEventLoop() {
        vector<ChangedRectangle> changes;
        while (true) {
            while (server.poll()) {
                for (int sock: server.sockets()) {
                    string msg = server.recv(sock);
                    cout << "sock(" << sock << "): " << msg << endl;
                    if (msg.empty()) cout << "Client disconnected" << endl;
                    if (msg.size() > 3) {
                        std::cout << "Received: " << msg << std::endl;
                        
                        vector<int> eventArgs;
                        stringstream ss(msg.substr(2, msg.size()));
                        string token;
                        while (getline(ss, token, ','))
                            eventArgs.push_back(stoi(token));

                        eventCallbacks.at(msg.substr(0, 2))(this, eventArgs);
                        
                    }
                }
            }
            if (server.sockets(true).empty()) continue;

            size_t size = changes.size();
            if (size) {
                for (int sock: server.sockets(true)) {
                    if (!server.send(sock, (const char*)&size, sizeof(size), 0)) {
                        server.disconnect(sock, "unable to send changed rectangles count");
                        break;
                    }
                    for (const ChangedRectangle& change: changes) {
                        ChangedRectangle resized = change.resize(
                            originWidth, originHeight, 
                            clientWidth, clientHeight
                        );
                        if (!resized.send(server, sock)) {
                            server.disconnect(sock, "partial image sending failed");
                            break;
                        }
                    }
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
