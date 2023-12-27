#pragma once

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

class TCP {
protected:
    sockaddr_in serverAddress{};

    int prepare(uint16_t port, in_addr_t addr = INADDR_ANY) {
        // Create socket
        int socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket == -1) throw runtime_error("Error creating socket");

        // Set up server address
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = addr; // Use any available network interface
        serverAddress.sin_port = htons(port); // Replace with your desired port

        return socket;
    }
public:

};