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
    const size_t MAX_BUFFER_SIZE = 1024; // 64000;
    static const int DEFAULT_POLL_TIMEOUT = 1;

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

    int poll(int timeout = DEFAULT_POLL_TIMEOUT) {
        int result = ::poll(pollfds.data(), pollfds.size(), timeout);
        if (result < 0) close();
        return result;
    }

    bool disconnect(int socket, const string& reason) {
        cerr << "Disconnect socket " << socket << ", reason: " << (reason.empty() ? "Unknown" : reason) << endl;
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
                    disconnect(socket, "chunk sending failed");
                    return false;
                } 
            }
            return true;
        }
        
        // simply send or throw error
        size_t sizechk;
        if (::send(socket, data, size, flags) == size) {
            ::recv(socket, (char*)&sizechk, sizeof(sizechk), flags);
            if (sizechk == size) return true;
        }
        disconnect(socket, "packet sending failed");
        return false;
    }

    ssize_t recv(int socket, char* data, size_t size, int flags) {
        ssize_t read = 0;

        // receive in chunks if the message is too long
        if (size > MAX_BUFFER_SIZE) {
            for (int i = 0; i < size; i += MAX_BUFFER_SIZE) {
                size_t minsize = min(size - i, MAX_BUFFER_SIZE);
                if (recv(socket, data + i, minsize, flags) != minsize) {
                    disconnect(socket, "chunk recieving failed");
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
        disconnect(socket, "packet recieving failed");
        return -1;
    }

    bool send(int socket, const string& msg, int flags = 0) {
        size_t size = msg.size();
        if (
            send(socket, (const char*)&size, sizeof(size), flags) && 
            send(socket, msg.c_str(), size, flags)
        ) return true;
        disconnect(socket, "string sending failed");
        return false;
    }

    string recv(int socket, int flags = 0) {
        size_t size;
        if (recv(socket, (char*)&size, sizeof(size), flags) != sizeof(size)) {
            disconnect(socket, "string size recieving failed");
            return "";
        }
        char buff[size + 1] = {0};
        if (recv(socket, buff, size, flags) != size) {
            disconnect(socket, "string contect recieving failed");
            return "";
        }
        return buff;
    }

    template<typename T>
    bool send_arr(int socket, const T& data, size_t size, int flags = 0) {
        if (!send(socket, (const char*)&size, sizeof(size), flags)) {
            disconnect(socket, "size sending failed");
            return false;
        }
        if (!send(socket, (const char*)data, size, flags)) {
            disconnect(socket, "data sending failed");
            return false;
        }
        return true;
    }

    size_t recv_arr(int socket, void** data, int flags = 0) {
        size_t size = 0;
        if (recv(socket, (char*)&size, sizeof(size), flags) != sizeof(size)) {
            disconnect(socket, "size recieving failed");
            return 0;
        }
        // size_t fullsize = sizeof(T) * size;
        *data = malloc(size);
        if (recv(socket, (char*)*data, size, flags) != size /*fullsize*/) {
            free(*data);
            disconnect(socket, "data recieving failed");
            return 0;
        }

        return size;
    }

    void free_arr(void** data) {
        free(*data);
        data = nullptr;
    }

    template<typename T>
    bool send_vector(int socket, const vector<T>& elems, int flags = 0) {

        size_t size = elems.size();
        if (!send(socket, (const char*)&size, sizeof(size), flags)) {
            disconnect(socket, "vector size sending failed");
            return false;
        }

        if (!send(socket, (const char*)elems.data(), sizeof(T) * size, flags)) {
            disconnect(socket, "vector data sending failed");
            return false;
        }
        // for (const T& elem: elems) 
        //     if (!send(socket, (const char*)&elem, sizeof(elem), flags)) {
        //         disconnect(socket, "vector sending failed");
        //         return false;
        //     }

        return true;
    }

    template<typename T>
    vector<T> recv_vector(int socket, int flags = 0) {

        size_t size;
        if (recv(socket, (char*)&size, sizeof(size), flags) != sizeof(size)) {
            disconnect(socket, "vector size recieving failed");
            return {};
        }
    
        vector<T> elems; 
        elems.resize(size);

        size_t fullsize = sizeof(T) * size;
        if (recv(socket, (char*)elems.data(), fullsize, flags) != fullsize) {
            disconnect(socket, "vector data recieving failed");
            return {};
        }
        // for (T& elem: elems)
        //     if (recv(socket, (char*)&elem, sizeof(elem), flags) != sizeof(elem)) {
        //         disconnect(socket, "vector recieving failed");
        //         return {};
        //     }

        return elems;
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

    int poll(int timeout = DEFAULT_POLL_TIMEOUT) {
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
