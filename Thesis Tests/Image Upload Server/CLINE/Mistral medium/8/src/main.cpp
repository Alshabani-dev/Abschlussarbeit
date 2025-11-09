#include <iostream>
#include <cstdlib>
#include <csignal>
#include "Server.h"

int main(int argc, char* argv[]) {
    // Ignore SIGPIPE to prevent crashes on broken connections
    std::signal(SIGPIPE, SIG_IGN);

    // Default port
    int port = 8080;

    // Parse command line arguments
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--port" && argc > 2) {
            port = std::stoi(argv[2]);
        }
    }

    std::cout << "Starting web server on port " << port << std::endl;

    try {
        Server server(port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
