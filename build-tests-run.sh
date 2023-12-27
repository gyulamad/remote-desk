g++ tests.cpp -o build/tests \
    -Ilibs/asio/asio/include \
    -lX11 -lXtst && \
./build/tests
