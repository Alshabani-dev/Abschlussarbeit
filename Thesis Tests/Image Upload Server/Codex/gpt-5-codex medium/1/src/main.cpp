#include "Server.h"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int parsePort(int argc, char* argv[]) {
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        }
    }
    return port;
}
} // namespace

int main(int argc, char* argv[]) {
    const int port = parsePort(argc, argv);
    try {
        Server server(port);
        std::cout << "Starting server on port " << port << '\n';
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Server failed: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
