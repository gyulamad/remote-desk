#pragma once

#include <string>

using namespace std;

class Communicator {
public:

    /**
     * @brief 
     * Implementation is optional,
     * server applications typically use to start waiting for clients.
     * 
     * @param info (optional) typically used for port number
     */
    virtual void listen(const string& info = "") {
        
    }

    /**
     * @brief 
     * Implementation is optional,
     * client applications use to connect to a server 
     * when multiple clients are possible in the implemented communication
     * 
     * @param addr (optional) typically used for server address in format "ipaddress:port"
     */
    virtual void connect(const string& addr = "") {
        
    }

    /**
     * @brief 
     * Implementation is optional,
     * use to close the connection greatfully when the implemented protocol requires
     * 
     * @param addr (optional) Typically used by server application to select a connected client. 
     *                        (No address means close all connections)
     */
    virtual void close(const string& addr = "") {
        
    }

    /**
     * @brief 
     * Recieving text data and set the addr where the data from.
     * Should be non-blocking (returns -1 if no data available)
     * 
     * @param data incoming data buffer
     * @param addr sender address (typical format is "ipaddress:port")
     * @return size_t incoming data size of -1 if there was no available data to read
     * @throw throws an exception when other error occurs
     */
    virtual ssize_t recv(string& data, string& addr) {
        throw runtime_error("Implementation is required");
    }

    /**
     * @brief 
     * Recieving binary data and set the addr where the data from.
     * Should be non-blocking (returns -1 if no data available)
     * 
     * @param data incoming data buffer
     * @param size maximum buffer size in bytes (reading can not reach)
     * @param addr sender address (typical format is "ipaddress:port")
     * @return size_t incoming data size of -1 if there was no available data to read
     * @throw throws an exception when other error occurs
     */
    virtual ssize_t recv(void* data, size_t size, string& addr) {
        throw runtime_error("Implementation is required");
    }
    
    /**
     * @brief 
     * Sending text data to the specified address
     * 
     * @param data data to send
     * @param addr (optional) recipient address (typical format is "ipaddress:port")
     * @return size_t sent data size (should match the data parameter size, otherwise sending was not full success)
     * @throw throws an exception when other error occurs
     */
    virtual size_t send(const string& data, const string& addr = "") {
        return send(data.c_str(), data.size(), addr);
    }


    /**
     * @brief 
     * Sending binary data to the specified address
     * 
     * @param data data to send
     * @param size data size in bytes
     * @param addr (optional) recipient address (typical format is "ipaddress:port")
     * @return size_t sent data size (should match the data parameter size, otherwise sending was not full success)
     * @throw throws an exception when other error occurs
     */
    virtual size_t send(const void* data, size_t size, const string& addr = "") {
        throw runtime_error("Implementation is required");
    }
};
