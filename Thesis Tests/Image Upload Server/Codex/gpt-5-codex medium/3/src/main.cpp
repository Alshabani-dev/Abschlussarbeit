#include "Server.h"

#include <iostream>
#include <string>

namespace {
int parsePortArgument(int argc, char* argv[]) {
    if (argc == 1) {
        return 8080;
    }
    if (argc == 3 && std::string(argv[1]) == "--port") {
        try {
            return std::stoi(argv[2]);
        } catch (const std::exception&) {
            throw std::invalid_argument("Port must be a valid integer.");
        }
    }
    throw std::invalid_argument("Usage: webserver [--port <port>]");
}
}  // namespace

int main(int argc, char* argv[]) {
    try {
        int port = parsePortArgument(argc, argv);
        Server server(port);
        std::cout << "Starting server on port " << port << std::endl;
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
