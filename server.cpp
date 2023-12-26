#include "src/DesktopServer.hpp"

int main() {
    DesktopServer desktopServer;
    desktopServer.runEventLoop();

    return 0;
}