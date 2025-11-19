#include "HttpServer.h"
#include <iostream>
#include <cstdlib>
#include <csignal>

HttpServer* globalServer = nullptr;

void signalHandler(int signal) {
    std::cout << "\nShutting down server..." << std::endl;
    if (globalServer) {
        globalServer->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 8080;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            ++i;
        } else if (arg == "-p" && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            ++i;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --port, -p PORT    Set server port (default: 8080)" << std::endl;
            std::cout << "  --help, -h         Show this help message" << std::endl;
            return 0;
        }
    }

    std::cout << "C++ Graph Plotting Server" << std::endl;
    std::cout << "=========================" << std::endl;

    HttpServer server(port);
    globalServer = &server;

    // Register signal handler for Ctrl+C
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    server.start();

    return 0;
}
