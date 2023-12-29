#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#include "src/tcp.hpp"

using namespace std;


class TCPServer: public TCPSocket {
public:

    sockaddr_in listen(uint16_t port) {
        sockaddr_in saddrin = prepare(port);

        // Bind the socket
        if (bind(mainSocket, (struct sockaddr*)&saddrin, sizeof(saddrin)) == -1) {
            ::close(mainSocket);
            throw runtime_error("Unable binding socket");
        }

        // Listen for incoming connections
        if (::listen(mainSocket, 5) == -1) {
            ::close(mainSocket);
            throw runtime_error("Unable to listen");
        }

        return saddrin;
    }

    int accept() {
        // Check for incoming connections on the server socket
        if (!(pollfds[0].revents & POLLIN)) return 0;
        int remoteSocket = ::accept(mainSocket, nullptr, nullptr);
        if (remoteSocket == -1) throw new runtime_error("Error accepting connection");
        
        // Add corresponding pollfd for the new client
        pollfd pfd;
        pfd.fd = remoteSocket;
        pfd.events = POLLIN;
        pollfds.push_back(pfd);
    
        return remoteSocket;
    }

};

int main() {
    const uint16_t port = 12358;
    TCPServer server;
    server.listen(port);
    cout << "Listening on port: " << port << endl;
    while (true) {
        cout << "poll..." << endl;
        if (server.poll()) {
            server.accept();
            for (int sock: server.sockets()) {
                cout << "start recv: " << sock << endl;
                string in = server.recv(sock);
                if (in.empty()) {
                    cout << "discon: " << sock << endl;
                    server.disconnect(sock);
                }
                cout << in << endl;
                string out = "Echo: " + in;
                server.send(sock, out);
            }
        }
        
    }

    return 0;
}

// class TCPServer: public TCP {
// protected:
//     int serverSocket;
//     vector<int> clientSockets;
//     struct pollfd fds;
// public:

//     const vector<int>& getClientSockets() const {
//         return clientSockets;
//     }

//     void listen(uint16_t port) {
//         // Create a socket
//         serverSocket = prepare(port);

//         // Bind the socket
//         if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
//             ::close(serverSocket);
//             throw runtime_error("Error binding socket");
//         }

//         // Listen for incoming connections
//         if (::listen(serverSocket, 5) == -1) {
//             ::close(serverSocket);
//             throw runtime_error("Error listening");
//         }

//         // --- initialization for polling ----
        
//         // Set up pollfd structure
//         fds.fd = serverSocket;
//         fds.events = POLLIN; // Check for incoming data
//     }

//     int pollConnect(int& result, int timeout = 1000) {

//         // Use poll to wait for a connection or timeout
//         int result = ::poll(&fds, 1, timeout);

//         // Error in poll?
//         if (result < 0) throw runtime_error("Error in poll: " + to_string(result));
//         if (!result) return 0;
        
//         int clientSocket = accept(serverSocket, nullptr, nullptr);
//         if (clientSocket == -1) {
//             ::close(serverSocket);
//             throw runtime_error("Error accepting connection");
//         }
//         clientSockets.push_back(clientSocket);

//         return clientSocket;
//     }

//     template<typename T>
//     vector<T> recv(int clientSocket) {
//         inputs.clear();

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
                
//         return inputs;
//     }

//     void send(int clientSocket, const char* data, size_t size) {
//         int i = 0;
//         for (; i < size; i += MAX_BUFFER_SIZE) ::send(clientSocket, data + i, MAX_BUFFER_SIZE, 0);
//         if (i < size) ::send(clientSocket, data + i, size - i, 0);
//     }

//     void close() {
//         for (int clientSocket: clientSockets) ::close(clientSocket);
//         ::close(serverSocket); 
//     }
// };

// int main() {    
//     try {
//         TCPServer server;
//         server.listen(12345);
//         cout << "Server listening on port 12345..." << endl;

//         int counter = 0;
//         while(true) {
//             int result;
//             int clientSocket;
//             cout << "Waiting for a client to connect..." << endl;
//             if (clientSocket = server.pollConnect(result, 1000))
//                 cout << "Client connected. client socket: " << clientSocket << ", result: " << result << endl;
//             else cout << "Timeout reached. No connection received." << endl;
//             while (clientSocket = server.poll(1000)) {
//                 cout << "Recieving client message from client socket: " << clientSocket << endl;
//                 cout << "Client message: " << server.recv<char>(clientSocket).data() << endl;
//                 cout << "Transfer complete." << endl;
//             }
//             // else cout << "Timeout reached. No incoming message received." << endl;

//             // Performing other processes...
//             string message = "Server is performing other processes... (" + to_string(++counter) + ")";
//             cout << message << endl;
//             if (counter % 7 == 0) {
//                 cout << "Server is sending broadcast..." << endl;
//                 for(int clientSocket: server.getClientSockets()) 
//                     server.send(clientSocket, message.c_str(), message.size());
//             }
//             usleep(1000000);

//         }

//         server.close();

//     } catch (exception &e) {
//         cout << "Exception: " << e.what() << endl;
//         return 1;
//     }

//     return 0;
// }
