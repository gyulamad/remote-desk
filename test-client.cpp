#include <iostream>
#include <fstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "src/tcp.hpp"

using namespace std;

#define MAX_BUFFER_SIZE 10

class TCPClient: public TCP {
protected:
    int clientSocket;
public:
    TCPClient(const char* addr, uint16_t port) {

        // Create a socket
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) throw runtime_error("Error creating socket");

        // Set up server address
        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = inet_addr(addr); // Replace with server IP address
        serverAddress.sin_port = htons(port); // Replace with server port number

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

        // // Get file name from user
        // std::string fileName = "a.txt";

        // // Send file name to the server
        // send(clientSocket, (fileName + "\n").c_str(), fileName.size() + 1, 0);

        // // Open the file for reading
        // std::ifstream inputFile(fileName, std::ios::binary);

        // if (!inputFile.is_open()) {
        //     perror("Error opening file for reading");
        //     close(clientSocket);
        //     return 1;
        // }

        // // Read and send file content
        // char buffer[MAX_BUFFER_SIZE];
        // while (inputFile.read(buffer, sizeof(buffer))) {
        //     send(clientSocket, buffer, inputFile.gcount(), 0);
        // }

        // // Check for errors or EOF
        // if (!inputFile.eof()) {
        //     perror("Error reading from file");
        //     close(clientSocket);
        //     return 1;
        // }

        // // Send the last chunk (if any)
        // if (inputFile.gcount() > 0) {
        //     send(clientSocket, buffer, inputFile.gcount(), 0);
        // }

        // // Close the file
        // inputFile.close();

        std::cout << "File transfer complete." << std::endl;

        // Clean up
        client.close();

    } catch (exception &e) {
        cout << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
