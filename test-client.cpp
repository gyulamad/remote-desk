#include <iostream>
#include <fstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "src/tcp.hpp"

using namespace std;

int main() {
    const char* addr = "127.0.0.1";
    const uint16_t port = 12358;
    TCPClient client;
    int socket = client.connect(addr, port);
    cout << "Connected to " << addr << ":" << port << endl;
    int counter = 0;
    while (true) {
        while (client.poll())
            // for (int socket: client.sockets()) // in case you are connecting multiple servers
                cout << "Recieved: " << client.recv(socket) << endl;

        usleep(1000000);
        counter++;
        string out = "It is a counter: " + to_string(counter);
        cout << "Sending: " << out << endl;
        client.send(socket, out);
    }

    return 0;
}
