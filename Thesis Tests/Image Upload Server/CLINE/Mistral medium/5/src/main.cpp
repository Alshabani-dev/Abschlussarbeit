#include "Server.h"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    int port = 8080;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        // Handle --port=8080 format
        if (arg.find("--port=") == 0) {
            std::string port_str = arg.substr(7); // Extract the port number after "--port="
            try {
                port = std::stoi(port_str);
                if (port <= 0 || port > 65535) {
                    std::cerr << "Invalid port number: " << port_str << ". Port must be between 1 and 65535." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Invalid port number: " << port_str << std::endl;
                return 1;
            }
        }
        // Handle -p 8080 format
        else if (arg == "-p" && i + 1 < argc) {
            std::string port_str = argv[i + 1];
            try {
                port = std::stoi(port_str);
                if (port <= 0 || port > 65535) {
                    std::cerr << "Invalid port number: " << port_str << ". Port must be between 1 and 65535." << std::endl;
                    return 1;
                }
                i++; // Skip the next argument as we've processed it
            } catch (const std::exception& e) {
                std::cerr << "Invalid port number: " << port_str << std::endl;
                return 1;
            }
        }
        // Handle direct port number
        else {
            try {
                port = std::stoi(arg);
                if (port <= 0 || port > 65535) {
                    std::cerr << "Invalid port number: " << arg << ". Port must be between 1 and 65535." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                // If it's not a number, ignore it (could be other arguments)
                continue;
            }
        }
    }

    std::cout << "Starting server on port " << port << std::endl;
    Server server(port);
    server.run();

    return 0;
}
