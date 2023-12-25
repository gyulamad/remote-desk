#pragma once

#include <string>
#include <unistd.h>

using namespace std;

typedef struct {
    sockaddr_in senderAddress;
    ssize_t length;
    string data;
} UDPMessage;
