clear
g++ test-client.cpp -o build/test-client \
    -lX11 -lXtst && \
./build/test-client
