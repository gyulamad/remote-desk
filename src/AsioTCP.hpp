#pragma once

#include <iostream>
#include <string>
#include <regex>
#include <asio.hpp>

#include "Communicator.hpp"

using namespace std;
using asio::ip::tcp;

class AsioTCP : public Communicator {
private:
    asio::io_context ioContext;
    tcp::socket socket;

public:

    AsioTCP(): socket(ioContext) {}

    virtual void listen(const string& info = "") override {
        tcp::acceptor acceptor(ioContext);
        tcp::endpoint endpoint(tcp::v4(), stoi(info));
        acceptor.open(endpoint.protocol());
        acceptor.bind(endpoint);
        acceptor.listen();

        acceptor.async_accept(socket, [this](const asio::error_code& ec) {
            if (!ec) {
                cout << "Connection accepted" << endl;
            }
        });

        ioContext.run();
    }

    
    virtual void connect(const string& addr = "") override {
        tcp::resolver::results_type endpoints;

        // Custom address parser
        std::regex addressRegex("^(.*):(\\d+)$");
        std::smatch match;

        if (std::regex_match(addr, match, addressRegex)) {
            // Assuming the first match is the full string, and the second and third are the host and port
            std::string host = match[1].str();
            std::string portStr = match[2].str();
            unsigned short port = static_cast<unsigned short>(std::stoi(portStr));

            tcp::resolver resolver(ioContext);
            endpoints = resolver.resolve(host, std::to_string(port));
        } else {
            throw runtime_error("Invalid address format");
        }

        // Synchronous connection
        asio::connect(socket, endpoints);

        // If we reach here, the connection was successful
        cout << "Connected to: " << socket.remote_endpoint().address().to_string() << ":" << socket.remote_endpoint().port() << endl;
    }

    virtual void close(const string& addr = "") override {
        // Implementation for closing connection
        socket.close();
    }

    virtual ssize_t recv(string& data, string& addr) override {
        asio::streambuf receiveBuffer;
        size_t bytesRead = asio::read_until(socket, receiveBuffer, '\0');

        // Extract the data from the buffer until the null terminator
        std::istream is(&receiveBuffer);
        std::getline(is, data, '\0');

        tcp::endpoint remoteEndpoint = socket.remote_endpoint();
        addr = remoteEndpoint.address().to_string() + ":" + to_string(remoteEndpoint.port());

        return bytesRead;
    }

    virtual ssize_t recv(void* data, size_t size, string& addr) override {
        size_t bytesRead = socket.receive(asio::buffer(data, size));

        tcp::endpoint remoteEndpoint = socket.remote_endpoint();
        addr = remoteEndpoint.address().to_string() + ":" + to_string(remoteEndpoint.port());

        return bytesRead;
    }

    virtual size_t send(const string& data, const string& addr) override {
        size_t bytesSent = socket.send(asio::buffer(data + '\0'));

        return bytesSent;
    }

    virtual size_t send(const void* data, size_t size, const string& addr) override {
        size_t bytesSent = socket.send(asio::buffer(data, size));

        return bytesSent;
    }
};
