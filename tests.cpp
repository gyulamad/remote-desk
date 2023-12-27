// #include "src/UDPClient.hpp"
// #include "src/UDPServer.hpp"
#include "src/AsioTCP.hpp"
#include "src/EventTrigger.hpp"
#include "src/ScreenshotManager.hpp"
#include "src/DesktopClient.hpp"


int main() {
    try {

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

        AsioTCP comm;
        DesktopClient desktopClient(comm);
        desktopClient.runEventLoop();
    } catch (exception &e) {
        cout << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
