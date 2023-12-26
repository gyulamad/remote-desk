#pragma once

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "UDPMessage.hpp"

class UDPServer {
public:
    UDPServer(int port) : port(port) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1) {
            std::cerr << "Error creating socket" << std::endl;
            return;
        }

        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(port);

        if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            std::cerr << "Error binding socket" << std::endl;
            close(sockfd);
            return;
        }
    }

    UDPMessage receive() {
        char receivedData[12000];
        socklen_t clientAddrLen = sizeof(clientAddr);

        ssize_t bytesReceived = recvfrom(sockfd, receivedData, sizeof(receivedData), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        receivedData[bytesReceived] = 0;
        
        return { clientAddr, bytesReceived, receivedData };
    }
    
    bool isDataAvailable() {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sockfd, &readSet);

        struct timeval timeout;
        timeout.tv_sec = 0;  // Set the timeout to 0 for non-blocking check
        timeout.tv_usec = 0;

        int result = select(sockfd + 1, &readSet, NULL, NULL, &timeout);

        return (result > 0 && FD_ISSET(sockfd, &readSet));
    }

    // void send(const char* message, const sockaddr* recipientAddress) {
    //     sendto(sockfd, message, strlen(message), 0, recipientAddress, sizeof(recipientAddress));
    // }
    void send(const char* message, size_t size, const sockaddr* recipientAddress) {
        sendto(sockfd, message, size, 0, recipientAddress, sizeof(*recipientAddress));
    }

    ~UDPServer() {
        close(sockfd);
    }

private:
    int sockfd;
    int port;
    sockaddr_in serverAddr, clientAddr;
};
