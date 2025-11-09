#include "Server.h"
#include <iostream>
#include <csignal>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " --port <port_number>" << std::endl;
    std::cout << "Example: " << programName << " --port 8080" << std::endl;
}

int main(int argc, char* argv[]) {
    // Ignore SIGPIPE to prevent server crashes on client disconnect
    std::signal(SIGPIPE, SIG_IGN);
    
    // Parse command line arguments
    int port = 8080; // Default port
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            try {
                port = std::stoi(argv[i + 1]);
                if (port < 1 || port > 65535) {
                    std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
                    return 1;
                }
                i++; // Skip next argument
            } catch (...) {
                std::cerr << "Error: Invalid port number" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    std::cout << "Starting C++ Web Server..." << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Public directory: public/" << std::endl;
    std::cout << "Upload directory: Data/" << std::endl;
    std::cout << std::endl;
    
    // Create and run server
    Server server(port);
    server.run();
    
    return 0;
}
