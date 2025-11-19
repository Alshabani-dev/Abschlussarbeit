#include "HttpServer.h"

#include <csignal>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        }
    }
    try {
        HttpServer server(port);
        std::cout << "Graph server listening on port " << port << std::endl;
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
