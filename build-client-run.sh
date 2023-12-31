clear
g++ -O3 client.cpp -o build/client \
    -lX11 -lXtst && \
./build/client
