#include <csignal>
#include <iostream>
#include <string>

#include "HttpServer.h"

namespace {
HttpServer *g_server = nullptr;

void handleSignal(int) {
    if (g_server) {
        std::cout << "\nShutting down server..." << std::endl;
        g_server->stop();
    }
}
} // namespace

int main(int argc, char **argv) {
    int port = 8080;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else {
            std::cerr << "Usage: " << argv[0] << " [--port <number>]" << std::endl;
            return 1;
        }
    }

    try {
        HttpServer server(port);
        g_server = &server;
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);
        server.start();
    } catch (const std::exception &ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
