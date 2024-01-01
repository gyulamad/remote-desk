clear
g++ -O3 server.cpp -o build/server \
    -lX11 -lXtst -ljpeg && \
./build/server
