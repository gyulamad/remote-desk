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

    vector<int> sockets(bool all = false) {
        vector<int> socks;
        size_t size = pollfds.size();
        for (size_t i = 1; i < size; i++) 
            if (all || pollfds[i].revents & POLLIN)
                socks.push_back(pollfds[i].fd);
        return socks;
    }

    int poll(int timeout = 100) {
        int result = ::poll(pollfds.data(), pollfds.size(), timeout);
        if (result < 0) close();
        return result;
    }

    bool disconnect(int socket) {
        size_t size = pollfds.size();
        size_t i = 0;
        for (; i < size; i++) if (pollfds[i].fd == socket) break;
        if (size == i) return false;
        ::close(socket);
        pollfds.erase(pollfds.begin() + i);
        return true;
    }

    void close() {
        for (const pollfd& pfd: pollfds) ::close(pfd.fd);
        pollfds.clear();
    }

    // Send in chunks if data is too long.
    bool send(int socket, const char* data, size_t size, int flags) {
        
        // send in chunks if the message is too long
        if (size > MAX_BUFFER_SIZE) {
            for (int i = 0; i < size; i += MAX_BUFFER_SIZE) {
                size_t minsize = min(size - i, MAX_BUFFER_SIZE);
                if (!send(socket, data + i, minsize, flags)) {
                    disconnect(socket);
                    return false;
                } 
            }
            return true;
        }
        
        // simply send or throw error
        if (::send(socket, data, size, flags) == size) {
            size_t sizechk;
            ::recv(socket, (char*)&sizechk, sizeof(sizechk), flags);
            if (sizechk == size) return true;
        }
        disconnect(socket);
        return false;
    }

    ssize_t recv(int socket, char* data, size_t size, int flags) {
        ssize_t read = 0;

        // receive in chunks if the message is too long
        if (size > MAX_BUFFER_SIZE) {
            for (int i = 0; i < size; i += MAX_BUFFER_SIZE) {
                size_t minsize = min(size - i, MAX_BUFFER_SIZE);
                if (recv(socket, data + i, minsize, flags) != minsize) {
                    disconnect(socket);
                    return -1;
                }
                read += minsize;
            }
            return read;
        }

        // simply receive or throw error
        read = ::recv(socket, data, size, flags);
        ::send(socket, (char*)&read, sizeof(read), flags);
        if (read == size) return read;
        disconnect(socket);
        return -1;
    }

    bool send(int socket, const string& msg, int flags = 0) {
        size_t size = msg.size();
        if (
            send(socket, (const char*)&size, sizeof(size), flags) && 
            send(socket, msg.c_str(), size, flags)
        ) return true;
        disconnect(socket);
        return false;
    }

    string recv(int socket, int flags = 0) {
        size_t size;
        if (recv(socket, (char*)&size, sizeof(size), flags) != sizeof(size)) {
            disconnect(socket);
            return "";
        }
        char buff[size + 1] = {0};
        if (recv(socket, buff, size, flags) != size) {
            disconnect(socket);
            return "";
        }
        return buff;
    }

    template<typename T>
    bool send_vector(int socket, const vector<T>& elems, int flags = 0) {
        size_t size = elems.size();
        const void* data = elems.data();
        if (
            send(socket, (const char*)&size, sizeof(size), flags) &&
            send(socket, (const char*)data, size * sizeof(T), flags)
        ) return true;
        disconnect(socket);
        return false;
    }

    template<typename T>
    vector<T> recv_vector(int socket, int flags = 0) {
        size_t size;
        if (recv(socket, (char*)&size, sizeof(size), flags) != sizeof(size)) disconnect(socket);
        else {
            T buff[size];
            size_t fullsize = sizeof(T) * size;
            if (recv(socket, (char*)buff, fullsize, flags) == fullsize) {
                vector<T> data(buff, buff + size);
                return data;
            }
            disconnect(socket);
        }
        return {};
    }
    
};



class TCPServer: public TCPSocket {
public:

    sockaddr_in listen(uint16_t port) {
        sockaddr_in saddrin = prepare(port);

        // Bind the socket
        if (bind(mainSocket, (struct sockaddr*)&saddrin, sizeof(saddrin)) == -1) {
            ::close(mainSocket);
            throw runtime_error("Unable binding socket to port: " + to_string(port));
        }

        // Listen for incoming connections
        if (::listen(mainSocket, 5) == -1) {
            ::close(mainSocket);
            throw runtime_error("Unable to listen on port: " + to_string(port));
        }

        return saddrin;
    }

    int accept() {
        // Check for incoming connections on the server socket
        if (!(pollfds[0].revents & POLLIN)) return 0;
        int remoteSocket = ::accept(mainSocket, nullptr, nullptr);
        if (remoteSocket == -1) return 0; // throw new runtime_error("Error accepting connection");
        
        // Add corresponding pollfd for the new client
        pollfd pfd;
        pfd.fd = remoteSocket;
        pfd.events = POLLIN;
        pollfds.push_back(pfd);
    
        return remoteSocket;
    }

    int poll(int timeout = 100) {
        int p = TCPSocket::poll(timeout);
        if (p) accept();
        return p;
    }

};


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

    int connect(const string& addr, uint16_t port) {
        return connect(addr.c_str(), port);
    }

};
