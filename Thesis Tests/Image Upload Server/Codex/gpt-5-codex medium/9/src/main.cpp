#include "Server.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc == 3 && std::string(argv[1]) == "--port") {
        port = std::atoi(argv[2]);
    }

    try {
        Server server(port);
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Server failed: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
