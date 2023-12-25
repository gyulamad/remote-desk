#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>

#include "UDPServer.hpp"
#include "EventTrigger.hpp"
#include "ScreenshotManager.hpp"

using namespace std;
using namespace chrono;

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

    UDPMessage clientMessage;
    bool clientJoined = false;
    ScreenshotManager screenshotManager = ScreenshotManager(100, 100);
    EventTrigger eventTrigger;
    UDPServer server = UDPServer(9876);
    long long captureNextAt = 0;
    long long captureFreq = 1000;
public:

    void runEventLoop() {
        while (true) {
            if (server.isDataAvailable()) {
                clientMessage = server.receive();
                clientJoined = true;
                if (clientMessage.length) {
                    std::cout << "Received: " << clientMessage.data << std::endl;
                    
                    vector<int> eventArgs;
                    stringstream ss(clientMessage.data.substr(2, clientMessage.length));
                    string token;
                    while (getline(ss, token, ','))
                        eventArgs.push_back(stoi(token));

                    eventCallbacks.at(clientMessage.data.substr(0, 2))(this, eventArgs);
                    
                }
            }

            long long now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            if (now >= captureNextAt) {
                cout << "Capture screen" << endl;
                const vector<ChangedRectangle>& changes = screenshotManager.captureChanges();
                char buffer[sizeof(ChangedRectangle)];
                for (const ChangedRectangle& change: changes) {
                    memcpy(buffer, &change, sizeof(ChangedRectangle));
                    server.send(buffer, (sockaddr*)&clientMessage.senderAddress);
                }
                captureNextAt = now + captureFreq;
            }
        }
    }
};

int main() {
    DesktopServer desktopServer;
    desktopServer.runEventLoop();

    return 0;
}