#include <iostream>
#include <fstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "src/tcp.hpp"

using namespace std;

#define MAX_BUFFER_SIZE 1024

class TCPClient: public TCP {
protected:
    int clientSocket;
public:
    TCPClient(const char* addr, uint16_t port) {

        // Create a socket
        clientSocket = prepare(port, inet_addr(addr));

        // Connect to the server
        if (::connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            ::close(clientSocket);
            throw runtime_error("Error connecting to server");
        }

    }

    void send(const char* data, size_t size) {
        int i = 0;
        for (; i < size; i += MAX_BUFFER_SIZE) ::send(clientSocket, data + i, MAX_BUFFER_SIZE, 0);
        if (i < size) ::send(clientSocket, data + i, size - i, 0);
    }

    void close() {
        ::close(clientSocket);
    }
};

int main() {
    try {
        const char* addr = "127.0.0.1";
        const uint16_t port = 12345;

        std::cout << "Connecting to the server..." << std::endl;
        TCPClient client(addr, port);

        string message = "This is a message that will be sent. I am writing a relatively long stuff here..";
        client.send(message.c_str(), message.size());
        std::cout << "Transfer complete." << std::endl;

        // Clean up
        client.close();

    } catch (exception &e) {
        cout << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
