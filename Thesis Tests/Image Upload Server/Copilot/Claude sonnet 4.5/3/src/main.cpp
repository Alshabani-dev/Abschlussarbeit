#include "Server.h"
#include <iostream>
#include <cstring>

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " --port <port_number>" << std::endl;
    std::cout << "Example: " << progName << " --port 8080" << std::endl;
}

int main(int argc, char* argv[]) {
    int port = 8080; // Default port
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            try {
                port = std::stoi(argv[i + 1]);
                if (port <= 0 || port > 65535) {
                    std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
                    return 1;
                }
                i++; // Skip next argument
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid port number" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    try {
        Server server(port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
