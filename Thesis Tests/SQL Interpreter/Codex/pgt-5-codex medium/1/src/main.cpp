#include <exception>
#include <iostream>
#include <string>

#include "Engine.h"
#include "HttpServer.h"

void printUsage(const char *program) {
    std::cout << "Usage: " << program << " [--script file | --web [port]]" << std::endl;
}

int main(int argc, char **argv) {
    try {
        Engine engine;
        if (argc == 1) {
            engine.repl();
            return 0;
        }
        std::string command = argv[1];
        if (command == "--script") {
            if (argc < 3) {
                std::cerr << "Missing script filename" << std::endl;
                return 1;
            }
            engine.executeScript(argv[2]);
            return 0;
        }
        if (command == "--web") {
            int port = 8080;
            if (argc >= 3) {
                port = std::stoi(argv[2]);
            }
            HttpServer server(engine);
            server.start(port);
            return 0;
        }
        if (command == "--help" || command == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        std::cerr << "Unknown option: " << command << std::endl;
        printUsage(argv[0]);
        return 1;
    } catch (const std::exception &ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
}
