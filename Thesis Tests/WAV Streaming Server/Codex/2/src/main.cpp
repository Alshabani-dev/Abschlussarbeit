#include "Server.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[]) {
    int port = 8080;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            ++i;
        }
    }

    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port. Please provide a value between 1 and 65535." << std::endl;
        return 1;
    }

    try {
        Server server(port);
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
