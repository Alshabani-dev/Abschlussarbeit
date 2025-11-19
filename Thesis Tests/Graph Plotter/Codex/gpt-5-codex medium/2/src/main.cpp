#include "HttpServer.h"

#include <csignal>
#include <iostream>

namespace {
HttpServer *gServer = nullptr;

void signalHandler(int) {
    if (gServer) {
        std::cout << "Signal received, shutting down..." << std::endl;
        gServer->stop();
    }
}
}

int main(int argc, char **argv) {
    int port = 8080;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (const std::exception &) {
            std::cerr << "Invalid port argument, fallback to 8080." << std::endl;
        }
    }

    HttpServer server(port);
    gServer = &server;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    server.start();
    gServer = nullptr;
    return 0;
}
