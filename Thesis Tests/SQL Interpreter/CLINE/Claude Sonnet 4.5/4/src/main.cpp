#include "Engine.h"
#include "HttpServer.h"
#include <iostream>
#include <string>
#include <memory>

void printUsage(const char* programName) {
    std::cout << "Usage:\n";
    std::cout << "  " << programName << "                 - Start interactive REPL mode\n";
    std::cout << "  " << programName << " <file.sql>      - Execute SQL from file\n";
    std::cout << "  " << programName << " --web [port]    - Start web server (default port: 8080)\n";
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
        std::string arg = argv[1];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--web") {
            // Start web server with default port
            int port = 8080;
            HttpServer server(port);
            
            server.setRequestHandler([&engine](const std::string& sql) {
                return engine.executeStatementWeb(sql);
            });
            
            server.start();
        } else {
            // Assume it's a SQL file
            engine.executeScript(arg);
        }
    } else if (argc == 3) {
        std::string arg1 = argv[1];
        
        if (arg1 == "--web") {
            // Start web server with custom port
            int port = std::stoi(argv[2]);
            HttpServer server(port);
            
            server.setRequestHandler([&engine](const std::string& sql) {
                return engine.executeStatementWeb(sql);
            });
            
            server.start();
        } else {
            std::cerr << "Error: Invalid arguments\n\n";
            printUsage(argv[0]);
            return 1;
        }
    } else {
        std::cerr << "Error: Too many arguments\n\n";
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
