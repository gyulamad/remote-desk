#include "src/DesktopClient.hpp"

int main() {
    try {
        const string addr = "127.0.0.1";
        const uint16_t port = 9876;
        cout << "Client connecting to " << addr << ":" << port << "..." << endl;
        DesktopClient desktopClient(addr, port); //(comm);
        desktopClient.runEventLoop();
    } catch (exception &e) {
        cout << "client error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
