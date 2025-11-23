#include "Engine.h"
#include "HttpServer.h"
#include <iostream>
#include <string>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Usage:\n";
    std::cout << "  " << programName << "                  # Interactive REPL mode\n";
    std::cout << "  " << programName << " <script.sql>     # Execute SQL script file\n";
    std::cout << "  " << programName << " --web [port]     # Start web server (default port: 8080)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << "\n";
    std::cout << "  " << programName << " script.sql\n";
    std::cout << "  " << programName << " --web\n";
    std::cout << "  " << programName << " --web 3000\n";
}

int main(int argc, char* argv[]) {
    Engine engine;
    
    // No arguments: REPL mode
    if (argc == 1) {
        engine.repl();
        return 0;
    }
    
    // Check for --web flag
    if (argc >= 2 && std::strcmp(argv[1], "--web") == 0) {
        int port = 8080; // Default port
        
        // Check if custom port provided
        if (argc >= 3) {
            try {
                port = std::stoi(argv[2]);
                if (port < 1 || port > 65535) {
                    std::cerr << "Error: Port must be between 1 and 65535\n";
                    return 1;
                }
            } catch (...) {
                std::cerr << "Error: Invalid port number\n";
                return 1;
            }
        }
        
        HttpServer server(&engine, port);
        server.start();
        return 0;
    }
    
    // Check for --help flag
    if (argc == 2 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0)) {
        printUsage(argv[0]);
        return 0;
    }
    
    // Script file mode
    if (argc == 2) {
        engine.executeScript(argv[1]);
        return 0;
    }
    
    // Unknown arguments
    std::cerr << "Error: Invalid arguments\n\n";
    printUsage(argv[0]);
    return 1;
}
