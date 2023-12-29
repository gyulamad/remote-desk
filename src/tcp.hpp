#pragma once

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

using namespace std;


template<typename T>
class TCPTransfer {
protected:
    int senderSocket;
    vector<T> message;
public:
    int getSenderSocket() const {
        return senderSocket;
    }
    const vector<T>& getMessage() const {
        return message;
    }
    void push(const T& input) {
        message.push_back(input);
    }
};

class TCPSocket {
protected:
    bool broadcast_enabled = false;
    int mainSocket = -1;
    vector<pollfd> pollfds;

    sockaddr_in prepare(uint16_t port, in_addr_t addr = INADDR_ANY) {
        // Create socket
        mainSocket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (mainSocket == -1) throw runtime_error("Error creating socket");

        // --- Initialization for polling ----

        pollfd pfd;
        pfd.fd = mainSocket;
        pfd.events = POLLIN;
        pollfds.push_back(pfd);
        if (addr != INADDR_ANY && pollfds.size() == 1)  // send() and recv() works from pollfds[1]
            pollfds.push_back(pfd);                     // and clients also will send to / recv from this

        // Set up server address
        sockaddr_in saddrin{};
        saddrin.sin_family = AF_INET;
        saddrin.sin_addr.s_addr = addr; // Use any available network interface
        saddrin.sin_port = htons(port); // Replace with your desired port

        return saddrin;
    }

public:
    const size_t MAX_BUFFER_SIZE = 1024;

    virtual ~TCPSocket() {
        close();
    }

    vector<int> sockets() {
        vector<int> socks;
        size_t size = pollfds.size();
        for (size_t i = 1; i < size; i++) 
            if (pollfds[i].revents & POLLIN)
                socks.push_back(pollfds[i].fd);
        return socks;
    }

    int poll(int timeout = 100) {
        int result = ::poll(pollfds.data(), pollfds.size(), timeout);
        if (result < 0) close(); // throw runtime_error("Polling failed: " + to_string(result));
        return result;
    }

    void disconnect(int socket) {
        size_t size = pollfds.size();
        size_t i = 0;
        for (; i < size; i++) if (pollfds[i].fd == socket) break;
        if (size == i) return; //throw runtime_error("Socket not found");
        ::close(socket);
        pollfds.erase(pollfds.begin() + i);
    }

    void close() {
        // TODO test it. is it working?? 
        for (const pollfd& pfd: pollfds) ::close(pfd.fd);
        pollfds.clear();
    }

    // Send in chunks if data is too long.
    ssize_t send(int socket, const char* data, size_t size, int flags) {
        ssize_t sent = 0;
        
        // send in chunks if the message is too long
        if (size > MAX_BUFFER_SIZE) {
            for (int i = 0; i < size; i += MAX_BUFFER_SIZE)
                sent += send(socket, data + i, min(size - i, MAX_BUFFER_SIZE), flags);
            if (sent == size) return sent;
            disconnect(socket);
            return -1;
        }
        
        // simply send or throw error
        sent = ::send(socket, data, size, flags);
        if (sent <= 0) disconnect(socket); //throw runtime_error("Sending failed");
        return sent;
    }

    ssize_t recv(int socket, char* data, size_t size, int flags) {
        ssize_t read = 0;

        // receive in chunks if the message is too long
        if (size > MAX_BUFFER_SIZE) {
            for (int i = 0; i < size; i += MAX_BUFFER_SIZE)
                read += recv(socket, data + i, min(size - i, MAX_BUFFER_SIZE), flags);
            if (read == size) return read;
            disconnect(socket);
            return -1;
        }

        // simply receive or throw error
        read = ::recv(socket, data, size, flags);
        if (read <= 0) disconnect(socket); //throw runtime_error("Receiving failed");
        return read;
    }

    void send(int socket, const string& msg, int flags = 0) {
        size_t size = msg.size();
        send(socket, (const char*)&size, sizeof(size), flags);
        send(socket, msg.c_str(), msg.size(), flags);
    }

    string recv(int socket, int flags = 0) {
        string msg;
        size_t size;
        size_t read = recv(socket, (char*)&size, sizeof(size), flags);
        if (read != sizeof(size)) {
            disconnect(socket);
            return "";
        }
        char buff[size + 1] = {0};
        recv(socket, buff, size, flags);
        return buff;
    }
    
};



// class TCP {
// protected:
//     const size_t MAX_BUFFER_SIZE = 1024;

//     sockaddr_in serverAddress{};

//     int prepare(uint16_t port, in_addr_t addr = INADDR_ANY) {
//         // Create socket
//         int socket = ::socket(AF_INET, SOCK_STREAM, 0);
//         if (socket == -1) throw runtime_error("Error creating socket");

//         // Set up server address
//         serverAddress.sin_family = AF_INET;
//         serverAddress.sin_addr.s_addr = addr; // Use any available network interface
//         serverAddress.sin_port = htons(port); // Replace with your desired port

//         return socket;INADDR_ANY
//     }
// public:

// };