#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#include "src/tcp.hpp"

using namespace std;

int main() {
    const uint16_t port = 12358;
    TCPServer server;
    server.listen(port);
    cout << "Listening on port: " << port << endl;
    while (true) {
        cout << "poll..." << endl;
        while (server.poll())
            for (int socket: server.sockets()) {
                string in = server.recv(socket);
                cout << "Recieved: " << in << endl;
                string out = "Echo: " + in;
                cout << "Sending echo: " << out << endl;
                server.send(socket, out);
            }
    }

    return 0;
}
