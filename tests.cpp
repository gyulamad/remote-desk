#include "UDPClient.hpp"
#include "UDPServer.hpp"
#include "EventTrigger.hpp"
#include "ScreenshotManager.hpp"
#include "WindowClient.hpp"


int main() {

    // --------------- ScreenshotManager -------------------

    ScreenshotManager screenshotManager(100, 100);

    // Example: Capture and display the result of the first capture
    vector<ChangedRectangle> firstCapture = screenshotManager.captureChanges();

    // ... (process the 'firstCapture' image as needed)

    // Example: Capture changes and process the resulting image
    vector<ChangedRectangle> changes = screenshotManager.captureChanges();

    // ... (process the 'changes' image as needed)


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

    UDPServer server(9877);
    UDPClient client("127.0.0.1", 9877);

    // Simple example: send a message from client to server
    client.send("Hello, Server!");


    // Server receives the message
    UDPMessage receivedMessage = server.receive();
    std::cout << "Server received from client: " << receivedMessage.data << std::endl;

    // Server sends a response to the client
    server.send("Hello, Client!", (sockaddr*)&receivedMessage.senderAddress);

    // --------------- WindowClient -------------------

    WindowClient windowClient;
    windowClient.runEventLoop();

    return 0;
}
