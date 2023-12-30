g++ -O3 client.cpp -o build/client \
    -Ilibs/asio/asio/include \
    -lX11 -lXtst && \
./build/client
