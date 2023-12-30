g++ -O3 server.cpp -o build/server \
    -Ilibs/asio/asio/include \
    -lX11 -lXtst && \
./build/server
