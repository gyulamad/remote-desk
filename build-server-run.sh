clear
libs/clib/build/builder -i ./server.cpp --libs " -lX11 -lXtst -ljpeg -lcrypto" -h
./build/release/build/release/hppcut/server
