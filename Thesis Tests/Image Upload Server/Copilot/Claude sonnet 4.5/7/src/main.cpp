#include "Server.h"
#include <iostream>
#include <string>
#include <cstring>

int main(int argc, char* argv[]) {
    int port = 8080; // Default port
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            try {
                port = std::stoi(argv[i + 1]);
                i++; // Skip next argument
            } catch (const std::exception& e) {
                std::cerr << "Invalid port number: " << argv[i + 1] << std::endl;
                return 1;
            }
        }
    }
    
    // Validate port number
    if (port < 1 || port > 65535) {
        std::cerr << "Port must be between 1 and 65535" << std::endl;
        return 1;
    }
    
    // Create and run server
    Server server(port);
    server.run();
    
    return 0;
}
