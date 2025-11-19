#include "HttpServer.h"

#include <csignal>
#include <iostream>

namespace {

HttpServer *gServer = nullptr;

void handleSignal(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        if (gServer) {
            gServer->stop();
        }
    }
}

}

int main(int argc, char **argv) {
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        }
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    HttpServer server(port, "public");
    gServer = &server;
    server.run();
    gServer = nullptr;

    return 0;
}
