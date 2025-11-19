#include "HttpServer.h"

#include <csignal>
#include <iostream>

namespace {
HttpServer* g_server = nullptr;

void handleSignal(int) {
    if (g_server) {
        g_server->stop();
    }
}
} // namespace

int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    HttpServer server(port);
    g_server = &server;
    try {
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }
    g_server = nullptr;

    return 0;
}
