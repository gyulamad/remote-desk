#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#include "src/tcp.hpp"

using namespace std;

#define MAX_BUFFER_SIZE 1024

class TCPServer: public TCP {
protected:
    int serverSocket;
    int clientSocket;
    struct pollfd fds;
public:

    void listen(uint16_t port) {
        // Create a socket
        serverSocket = prepare(port);

        // Bind the socket
        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            ::close(serverSocket);
            throw runtime_error("Error binding socket");
        }

        // Listen for incoming connections
        if (::listen(serverSocket, 5) == -1) {
            ::close(serverSocket);
            throw runtime_error("Error listening");
        }

        // --- initialization for polling ----
        
        // Set up pollfd structure
        fds.fd = serverSocket;
        fds.events = POLLIN; // Check for incoming data
    }

    int poll(int timeout = 1000) {

        // Use poll to wait for a connection or timeout
        int result = ::poll(&fds, 1, timeout);

        // Error in poll?
        if (result < 0) throw runtime_error("Error in poll: " + to_string(result));
        if (!result) return 0;
        
        clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == -1) {
            ::close(serverSocket);
            throw runtime_error("Error accepting connection");
        }

        return result;
    }

    template<typename T>
    vector<T> recv() {

        vector<T> inputs;

        // Receive and write file content
        T buffer[MAX_BUFFER_SIZE / sizeof(T)];
        int bytesRead;
        while ((bytesRead = ::recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
            for (int i=0, n=0; i<bytesRead; i+=sizeof(T), n++)
                inputs.push_back(buffer[n]);
                
        return inputs;
    }

    void close() {
        ::close(clientSocket);
        ::close(serverSocket); 
    }
};

int main() {    
    try {
        TCPServer server;
        server.listen(12345);
        std::cout << "Server listening on port 12345..." << std::endl;

        while(true) {
            std::cout << "Waiting for a client to connect..." << std::endl;
            
            // Wait for a client to connect
            while (server.poll(1000)) {
                std::cout << "Client connected, recieving..." << std::endl;
                cout << server.recv<char>().data() << endl;
                std::cout << "Transfer complete." << std::endl;
            }

            std::cout << "Timeout reached. No connection received." << std::endl;
        }

        server.close();

    } catch (exception &e) {
        cout << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
