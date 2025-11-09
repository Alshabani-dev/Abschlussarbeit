#include "Server.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    int port = 8080;

    if (argc == 3) {
        std::string flag = argv[1];
        if (flag == "--port") {
            port = std::atoi(argv[2]);
        }
    }

    try {
        Server server(port);
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
