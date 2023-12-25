#pragma once

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "UDPMessage.hpp"

class UDPClient {
public:
    UDPClient(const char* serverIp, int serverPort) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1) {
            std::cerr << "Error creating socket" << std::endl;
            return;
        }

        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(serverIp);
        serverAddr.sin_port = htons(serverPort);
    }

    UDPMessage receive() {
        char receivedData[12000];
        socklen_t serverAddrLen = sizeof(serverAddr);

        ssize_t bytesReceived = recvfrom(sockfd, receivedData, sizeof(receivedData), 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
        receivedData[bytesReceived] = 0;
        
        return { serverAddr, bytesReceived, receivedData };
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

    void send(const char* data) {
        sendto(sockfd, data, strlen(data), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    }

    ~UDPClient() {
        close(sockfd);
    }

private:
    int sockfd;
    sockaddr_in serverAddr;
};
