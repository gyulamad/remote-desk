#pragma once

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

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
