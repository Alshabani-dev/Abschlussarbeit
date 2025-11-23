#include "Engine.h"
#include "HttpServer.h"
#include <iostream>
#include <string>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << programName << "              - Start in interactive mode (REPL)" << std::endl;
    std::cout << "  " << programName << " <file.sql>   - Execute SQL script from file" << std::endl;
    std::cout << "  " << programName << " --web [port] - Start web server (default port: 8080)" << std::endl;
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
    } else if (argc == 2) {
        std::string arg = argv[1];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--web") {
            // Web mode with default port
            int port = 8080;
            HttpServer server(port, [&engine](const std::string& sql) {
                return engine.executeStatementWeb(sql);
            });
            server.start();
        } else {
            // Script file mode
            engine.executeScript(arg);
        }
    } else if (argc == 3) {
        std::string arg = argv[1];
        
        if (arg == "--web") {
            // Web mode with custom port
            int port = std::stoi(argv[2]);
            HttpServer server(port, [&engine](const std::string& sql) {
                return engine.executeStatementWeb(sql);
            });
            server.start();
        } else {
            std::cerr << "Error: Invalid arguments" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    } else {
        std::cerr << "Error: Too many arguments" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
