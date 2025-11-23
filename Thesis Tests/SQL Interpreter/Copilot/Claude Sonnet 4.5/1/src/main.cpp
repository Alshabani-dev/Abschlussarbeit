#include "Engine.h"
#include "HttpServer.h"
#include <iostream>
#include <cstring>
#include <cstdlib>

void printUsage(const char* programName) {
    std::cout << "Usage:\n";
    std::cout << "  " << programName << "                    - Start interactive REPL mode\n";
    std::cout << "  " << programName << " <script.sql>       - Execute SQL from script file\n";
    std::cout << "  " << programName << " --web [port]       - Start web server (default port: 8080)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << "\n";
    std::cout << "  " << programName << " script.sql\n";
    std::cout << "  " << programName << " --web\n";
    std::cout << "  " << programName << " --web 3000\n";
}

int main(int argc, char* argv[]) {
    Engine engine;
    
    if (argc == 1) {
        // No arguments - start REPL
        engine.repl();
    } else if (argc == 2) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[1], "--web") == 0) {
            // Start web server on default port
            HttpServer server(&engine, 8080);
            server.start();
        } else {
            // Execute script file
            engine.executeScript(argv[1]);
        }
    } else if (argc == 3 && strcmp(argv[1], "--web") == 0) {
        // Start web server on custom port
        int port = std::atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Error: Invalid port number. Must be between 1 and 65535.\n";
            return 1;
        }
        HttpServer server(&engine, port);
        server.start();
    } else {
        std::cerr << "Error: Invalid arguments.\n\n";
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
