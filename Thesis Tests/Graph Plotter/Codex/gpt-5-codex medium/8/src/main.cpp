#include "HttpServer.h"

#include <csignal>
#include <iostream>
#include <string>

namespace {
HttpServer *gServerInstance = nullptr;

void handleSignal(int) {
    if (gServerInstance) {
        std::cout << "\nShutting down server...\n";
        gServerInstance->stop();
    }
}
} // namespace

int main(int argc, char **argv) {
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            try {
                port = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "Invalid port value\n";
                return 1;
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: graphserver [--port <number>]\n";
            return 0;
        }
    }

    HttpServer server(port);
    gServerInstance = &server;
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    server.run();
    gServerInstance = nullptr;
    return 0;
}
