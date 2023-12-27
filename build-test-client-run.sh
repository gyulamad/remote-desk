g++ test-client.cpp -o build/test-client \
    -Ilibs/asio/asio/include \
    -lX11 -lXtst && \
./build/test-client
