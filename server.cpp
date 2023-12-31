#include "src/DesktopServer.hpp"

int main() {
    try {
        const uint16_t port = 9876;
        TCPServer server;
        server.listen(port);
        cout << "Server is listening on port: " << port << endl;
        DesktopServer desktopServer(server);
        desktopServer.runEventLoop();
    } catch (exception &e) {
        cout << "server error: " << e.what() << endl;
        return 1;
    }
    return 0;
}