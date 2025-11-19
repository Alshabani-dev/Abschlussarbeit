#include "HttpServer.h"

#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace {
void signalHandler(int) {
    std::cout << "\nShutting down server..." << std::endl;
    std::_Exit(0);
}
}

int main(int argc, char **argv) {
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        }
    }

    std::signal(SIGINT, signalHandler);

    try {
        HttpServer server(port, "public");
        server.start();
    } catch (const std::exception &ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
