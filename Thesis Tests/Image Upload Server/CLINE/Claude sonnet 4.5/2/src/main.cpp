#include "Server.h"
#include <iostream>
#include <string>
#include <cstring>

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " --port <port_number>" << std::endl;
    std::cout << "Example: " << program_name << " --port 8080" << std::endl;
}

int main(int argc, char* argv[]) {
    int port = 8080; // Default port
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            try {
                port = std::stoi(argv[i + 1]);
                i++; // Skip next argument
            } catch (const std::exception& e) {
                std::cerr << "Invalid port number: " << argv[i + 1] << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << argv[i] << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Validate port range
    if (port < 1 || port > 65535) {
        std::cerr << "Port number must be between 1 and 65535" << std::endl;
        return 1;
    }
    
    try {
        Server server(port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
