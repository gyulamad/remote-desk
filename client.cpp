#include "libs/clib/clib/chiper.hpp"

#include "src/DesktopClient.hpp"

int main() {
    try {
        const bool keygen = false;
        const string cliid = "testcli1";
        const string pubkey = "keys/" + cliid + ".pubkey.pem";
        const string privkey = "keys/" + cliid + ".privkey.pem";
        if (keygen) {
            const int rsabits = 2048;
            rsa_generate(privkey, pubkey, rsabits);
            cout << "Public key generated, send over to the server: " + pubkey << endl;
            return 0;
        }
        const string addr = "127.0.0.1";
        const uint16_t port = 9876;
        cout << "Client connecting to " << addr << ":" << port << "..." << endl;
        DesktopClient desktopClient(addr, port, cliid); //(comm);
        desktopClient.runEventLoop();
    } catch (exception &e) {
        cout << "client error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
