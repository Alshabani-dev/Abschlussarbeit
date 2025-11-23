#include "Engine.h"
#include "HttpServer.h"
#include <iostream>
#include <string>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Usage:\n";
    std::cout << "  " << programName << "                    # Start interactive REPL\n";
    std::cout << "  " << programName << " <script.sql>       # Execute SQL script file\n";
    std::cout << "  " << programName << " --web [port]       # Start web server (default port: 8080)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << "\n";
    std::cout << "  " << programName << " script.sql\n";
    std::cout << "  " << programName << " --web\n";
    std::cout << "  " << programName << " --web 3000\n";
}

int main(int argc, char* argv[]) {
    try {
        Engine engine;
        
        if (argc == 1) {
            // No arguments - start REPL
            engine.repl();
        } else if (argc == 2) {
            std::string arg = argv[1];
            
            if (arg == "--help" || arg == "-h") {
                printUsage(argv[0]);
                return 0;
            } else if (arg == "--web") {
                // Start web server with default port
                HttpServer server(8080);
                server.start();
            } else {
                // Execute script file
                engine.executeScript(arg);
            }
        } else if (argc == 3) {
            std::string arg1 = argv[1];
            
            if (arg1 == "--web") {
                // Start web server with custom port
                int port = std::stoi(argv[2]);
                if (port < 1 || port > 65535) {
                    std::cerr << "Error: Invalid port number. Must be between 1 and 65535.\n";
                    return 1;
                }
                HttpServer server(port);
                server.start();
            } else {
                std::cerr << "Error: Invalid arguments\n";
                printUsage(argv[0]);
                return 1;
            }
        } else {
            std::cerr << "Error: Too many arguments\n";
            printUsage(argv[0]);
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
