#include "Engine.h"
#include "HttpServer.h"
#include <iostream>
#include <string>
#include <cstdlib>

void printUsage(const char* programName) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << programName << "                    # Interactive REPL mode" << std::endl;
    std::cout << "  " << programName << " <script.sql>      # Execute SQL script file" << std::endl;
    std::cout << "  " << programName << " --web [port]      # Start web server (default port: 8080)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << std::endl;
    std::cout << "  " << programName << " script.sql" << std::endl;
    std::cout << "  " << programName << " --web" << std::endl;
    std::cout << "  " << programName << " --web 3000" << std::endl;
}

int main(int argc, char* argv[]) {
    Engine engine;
    
    if (argc == 1) {
        // No arguments - start REPL mode
        engine.repl();
    }
    else if (argc == 2) {
        std::string arg1(argv[1]);
        
        if (arg1 == "--help" || arg1 == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg1 == "--web") {
            // Start web server with default port
            HttpServer server(8080, &engine);
            server.start();
        }
        else {
            // Execute script file
            engine.executeScript(arg1);
        }
    }
    else if (argc == 3) {
        std::string arg1(argv[1]);
        
        if (arg1 == "--web") {
            // Start web server with custom port
            int port = std::atoi(argv[2]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Error: Invalid port number. Must be between 1 and 65535." << std::endl;
                return 1;
            }
            
            HttpServer server(port, &engine);
            server.start();
        }
        else {
            printUsage(argv[0]);
            return 1;
        }
    }
    else {
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
