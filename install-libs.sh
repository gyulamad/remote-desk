mkdir libs
cd libs

git clone https://github.com/chriskohlhoff/asio.git
cd asio
git checkout asio-1-28-2 
cd asio
./autogen.sh
./configure --with-boost=no
make
cd ..
cd ..


