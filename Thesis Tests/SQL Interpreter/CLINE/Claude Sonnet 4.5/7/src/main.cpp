#include <iostream>
#include <string>
#include <cstring>
#include "Engine.h"
#include "HttpServer.h"

void printUsage(const char* programName) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << programName << "                    # Start interactive REPL" << std::endl;
    std::cout << "  " << programName << " <script.sql>       # Execute SQL script file" << std::endl;
    std::cout << "  " << programName << " --web [port]       # Start web server (default port: 8080)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << std::endl;
    std::cout << "  " << programName << " init.sql" << std::endl;
    std::cout << "  " << programName << " --web" << std::endl;
    std::cout << "  " << programName << " --web 3000" << std::endl;
}

int main(int argc, char* argv[]) {
    Engine engine;
    
    if (argc == 1) {
        // No arguments: start REPL
        engine.repl();
    }
    else if (argc == 2) {
        std::string arg = argv[1];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
        }
        else if (arg == "--web") {
            // Start web server on default port
            HttpServer server(8080, &engine);
            server.start();
        }
        else {
            // Execute script file
            engine.executeScript(arg);
        }
    }
    else if (argc == 3) {
        std::string arg1 = argv[1];
        
        if (arg1 == "--web") {
            // Start web server on specified port
            int port = std::atoi(argv[2]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Error: Invalid port number. Must be between 1 and 65535." << std::endl;
                return 1;
            }
            
            HttpServer server(port, &engine);
            server.start();
        }
        else {
            std::cerr << "Error: Invalid arguments" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    else {
        std::cerr << "Error: Too many arguments" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
