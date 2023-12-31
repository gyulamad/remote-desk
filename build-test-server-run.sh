clear
g++ test-server.cpp -o build/test-server \
    -lX11 -lXtst && \
./build/test-server
