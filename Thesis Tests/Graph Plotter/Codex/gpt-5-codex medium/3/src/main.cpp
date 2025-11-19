#include "HttpServer.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    int port = 8080;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else {
            std::cerr << "Usage: " << argv[0] << " [--port <number>]\n";
            return 1;
        }
    }

    HttpServer server(port);
    server.run();
    return 0;
}
