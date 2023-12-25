#include <iostream>
#include <sstream>
#include <vector>
#include <map>

#include "UDPServer.hpp"
#include "EventTrigger.hpp"
#include "ScreenshotManager.hpp"

class DesktopServer {
protected:

    typedef void (*EventCallback)(DesktopServer*, const vector<int>&);

    const map<string, EventCallback> eventCallbacks = {
        { "kp", triggerKeyPress },
        { "kr", triggerKeyRelease },
        { "mp", triggerMousePress },
        { "mr", triggerMouseRelease },
        { "mm", triggerMouseMove },
        { "wr", adaptWindowResize }, // TODO: may YAGNI now but we may want to send less pixel if the client window is small
    };

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

    ScreenshotManager screenshotManager;
    EventTrigger eventTrigger;
    UDPServer server = UDPServer(9876);
public:

    void runEventLoop() {
        while (true) {
            if (server.isDataAvailable()) {
                UDPMessage message = server.receive();
                if (message.length) {
                    std::cout << "Received: " << message.data << std::endl;
                    
                    vector<int> eventArgs;
                    stringstream ss(message.data.substr(2, message.length));
                    string token;
                    while (getline(ss, token, ','))
                        eventArgs.push_back(stoi(token));

                    eventCallbacks.at(message.data.substr(0, 2))(this, eventArgs);
                    
                }
            }

            screenshotManager.captureChanges();

            usleep(50000);
        }
    }
};

int main() {
    DesktopServer desktopServer;
    desktopServer.runEventLoop();

    return 0;
}