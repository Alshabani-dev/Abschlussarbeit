#include <csignal>
#include <iostream>
#include <string>

#include "HttpServer.h"

namespace {
HttpServer* gServer = nullptr;

void handleSignal(int) {
    if (gServer) {
        std::cout << "\nShutting down server..." << std::endl;
        gServer->stop();
    }
}
}

int main(int argc, char* argv[]) {
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        }
    }

    try {
        HttpServer server(port, "public");
        gServer = &server;
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);
        server.start();
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
