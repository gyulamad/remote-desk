#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>

#include "Communicator.hpp"
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
        // TODO
    }

    vector<string> clientAddresses;
    bool clientJoined = false;
    ScreenshotManager screenshotManager = ScreenshotManager(20, 20);
    EventTrigger eventTrigger;
    Communicator& server;
    long long captureNextAt = 0;
    long long captureFreq = 5000;
public:

    DesktopServer(Communicator& server): server(server) {
        const string port = "9876";
        cout << "server listening on port: " << port << endl;
        server.listen(port);
    }

    ~DesktopServer() {}

    void runEventLoop() {
        while (true) {
            // if (server.isDataAvailable()) {
                // const size_t commBuffSizeMax = 60000;
                string receivedData; //(commBuffSizeMax, '\0');
                string senderAddress;
                ssize_t receivedLength = server.recv(receivedData, senderAddress);
                clientAddresses.push_back(senderAddress);
                clientJoined = true;
                cout << "Client joined" << endl;
                if (receivedLength > 3) {
                    std::cout << "Received: " << receivedData << std::endl;
                    
                    vector<int> eventArgs;
                    stringstream ss(receivedData.substr(2, receivedLength));
                    string token;
                    while (getline(ss, token, ','))
                        eventArgs.push_back(stoi(token));

                    eventCallbacks.at(receivedData.substr(0, 2))(this, eventArgs);
                    
                }
            // }

            if (!clientJoined) continue;

            long long now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            if (now >= captureNextAt) {
                cout << "Capture screen" << endl;
                const vector<ChangedRectangle>& changes = screenshotManager.captureChanges();
                
                int i=0;
                for (const ChangedRectangle& change: changes) {
                    i++;
                    try {
                        string outs = change.toString();
                        cout << "Sending image part (" << changes.size() << "/" << i << "): (" << outs.size() << "): [" << outs.substr(0, 80) << "...]" << endl; 
                        for (const string& clientAddress: clientAddresses) 
                            server.send(outs, clientAddress);
                        cout << "Sent." << endl;
                        usleep(1000);
                    } catch (exception &e) {
                        cout << "Image sending error: " << e.what() << endl;
                    }
                }
                captureNextAt = now + captureFreq;
            }
        }
    }
};
