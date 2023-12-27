#include "src/AsioTCP.hpp"
#include "src/DesktopClient.hpp"

int main() {
    try {
        AsioTCP comm;
        DesktopClient desktopClient(comm);
        desktopClient.runEventLoop();
    } catch (exception &e) {
        cout << "client error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
