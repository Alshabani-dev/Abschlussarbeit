#include "Server.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    int port = 8080;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            try {
                port = std::stoi(argv[++i]);
            } catch (const std::exception& ex) {
                std::cerr << "Invalid port value: " << ex.what() << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Usage: " << argv[0] << " [--port <number>]" << std::endl;
            return 1;
        }
    }

    try {
        Server server(port);
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
