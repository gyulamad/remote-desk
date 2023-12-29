#include <iostream>
#include <fstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "src/tcp.hpp"

using namespace std;

class TCPClient: public TCPSocket {
public:
    int connect(const char* addr, uint16_t port) {
        // Create a socket
        sockaddr_in saddrin = prepare(port, inet_addr(addr));

        // Connect to the server
        if (::connect(mainSocket, (struct sockaddr*)&saddrin, sizeof(saddrin)) == -1) {
            ::close(mainSocket);
            throw runtime_error("Error connecting to server");
        }

        return mainSocket;
    }

};

int main() {
    const char* addr = "127.0.0.1";
    const uint16_t port = 12358;
    TCPClient client;
    int socket = client.connect(addr, port);
    cout << "Connected to " << addr << ":" << port << endl;
    int counter = 0;
    while (true) {
        if (client.poll())
            cout << client.recv(socket) << endl;

        usleep(1000000);
        counter++;
        client.send(socket, to_string(counter));
    }

    return 0;
}

// class TCPClient: public TCP {
// protected:
//     int clientSocket;
// public:
//     void connect(const char* addr, uint16_t port) {

//         // Create a socket
//         clientSocket = prepare(port, inet_addr(addr));

//         // Connect to the server
//         if (::connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
//             ::close(clientSocket);
//             throw runtime_error("Error connecting to server");
//         }

//     }

//     int poll(int timeout = 1000) {

//         // Use poll to check if there is any incoming message or timeout
//         int result = 0; // = ::poll(&fds, 1, timeout);

//         // Error in poll?
//         if (result < 0) throw runtime_error("Error in poll: " + to_string(result));
//         if (!result) return 0;
        
//         // clientSocket = accept(serverSocket, nullptr, nullptr);
//         // if (clientSocket == -1) {
//         //     ::close(serverSocket);
//         //     throw runtime_error("Error accepting connection");
//         // }

//         return result;
//     }

//     template<typename T>
//     vector<T> recv() {

//         vector<T> inputs;

//         // Receive and write file content
//         cout << "max/sizeT:" << MAX_BUFFER_SIZE / sizeof(T) << endl; 
//         T buffer[MAX_BUFFER_SIZE / sizeof(T)];
//         int bytesRead;
//         int bytesReadSum = 0;
//         while ((bytesRead = ::recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
//             cout << bytesRead << endl;
//             bytesReadSum += bytesRead;
//             for (int i = 0, n = 0; i < bytesRead; i += sizeof(T), n++) {
//                 inputs.push_back(buffer[n]);
//                 cout << "size:" << inputs.size() << endl;
//             }
//         }
//         cout << "sum:" << bytesReadSum << endl;
//         cout << inputs.data() << endl;
                
//         return inputs;
//     }

//     void send(const char* data, size_t size) {
//         int i = 0;
//         for (; i < size; i += MAX_BUFFER_SIZE) ::send(clientSocket, data + i, MAX_BUFFER_SIZE, 0);
//         if (i < size) ::send(clientSocket, data + i, size - i, 0);
//     }

//     void close() {
//         ::close(clientSocket);
//     }
// };

// int main() {
//     try {
//         const char* addr = "127.0.0.1";
//         const uint16_t port = 12345;

//         TCPClient client;
//         cout << "Connecting to the server..." << endl;
//         client.connect(addr, port);

//         // string message = "This is a message that will be sent. I am writing a relatively long stuff here..";
//         // client.send(message.c_str(), message.size());
//         // cout << "Transfer complete." << endl;
        
//         int counter = 0;
//         while(true) {
//             cout << "Checking for incoming message..." << endl;
//             if (client.poll(1000)) {
//                 cout << "Recieving message from the server..." << endl;
//                 cout << client.recv<char>().data() << endl;
//                 cout << "Transfer complete." << endl;
//             }
//             cout << "Timeout reached. No incoming message received." << endl;

//             // Performing other processes...
//             string message = "Client is performing other processes... (" + to_string(++counter) + ")";
//             cout << message << endl;
//             if (counter % 5 == 0) {
//                 cout << "Client is sending..." << endl;
//                 client.send(message.c_str(), message.size());
//             }
//             usleep(1000000);
//         }

//         // Clean up
//         client.close();

//     } catch (exception &e) {
//         cout << "Exception: " << e.what() << endl;
//         return 1;
//     }

//     return 0;
// }

