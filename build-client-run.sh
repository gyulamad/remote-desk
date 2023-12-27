g++ client.cpp -o build/client \
    -Ilibs/asio/asio/include \
    -lX11 -lXtst && \
./build/client
