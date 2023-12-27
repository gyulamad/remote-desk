#include "src/AsioTCP.hpp"
#include "src/DesktopServer.hpp"

int main() {
    try {
        AsioTCP server;
        DesktopServer desktopServer(server);
        desktopServer.runEventLoop();
    } catch (exception &e) {
        cout << "server error: " << e.what() << endl;
        return 1;
    }
    return 0;
}