#include <iostream>
#include <asio.hpp>
#include <fstream>

#include "src/ScreenshotManager.hpp"

using namespace std;
using namespace asio;

class DesktopStream {
protected:

    io_context ioContext;
    ip::tcp::acceptor acceptor;
    ip::tcp::socket socket;

    void Accept() {
        acceptor.async_accept(socket, [this](error_code ec) {
            if (!ec) {
                // Handle the new connection, start sending the video stream.
                cout << "Client joined" << endl;
                StartSendingVideo();
            }

            // Continue accepting new connections.
            Accept();
        });
    }

    void StartSendingVideo() {
        // Implement logic to send the real-time video stream.
        // You can use socket.async_write_some for asynchronous writing.
        ScreenshotManager screenshotManager(100, 100);
        const vector<ChangedRectangle>& changes = screenshotManager.captureChanges();
        for (const ChangedRectangle& change: changes) {
            // Serialize the ChangedRectangle into a buffer
            std::vector<char> buffer = change.serialize();

            // Asynchronously write the buffer to the socket
            asio::async_write(
                socket,
                asio::buffer(buffer),
                [this, buffer](std::error_code ec, std::size_t /*length*/) {
                    if (ec) {
                        if (ec == asio::error::eof) {
                            // Connection closed by the client, handle it appropriately.
                            std::cout << "Connection closed by the client." << std::endl;
                        } else {
                            std::cerr << "Error sending data: " << ec.message() << std::endl;
                        }
                    } else {
                        // Data sent successfully, you can handle success here
                        std::cout << "Changed rectangle sent" << std::endl;
                    }
                });
        }
    }

public:

    DesktopStream(short port): 
        acceptor(ioContext, ip::tcp::endpoint(ip::tcp::v4(), port)),
        socket(ioContext)
    {
        cout << "Desktop stream is listening on port: " << port << endl;
        Accept();
        ioContext.run();
    }
};


int main() {

    try {
        DesktopStream desktopStream(12345);
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}