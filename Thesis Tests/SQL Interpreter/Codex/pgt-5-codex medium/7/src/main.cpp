#include <cstdlib>
#include <iostream>
#include <string>

#include "Engine.h"
#include "HttpServer.h"

void printUsage() {
    std::cout << "Usage: minisql [--script file] [--web port]\n";
}

int main(int argc, char **argv) {
    Engine engine;
    if (argc == 1) {
        engine.repl();
        return 0;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--script") {
        if (argc < 3) {
            printUsage();
            return 1;
        }
        engine.executeScript(argv[2]);
        return 0;
    }
    if (arg1 == "--web") {
        if (argc < 3) {
            printUsage();
            return 1;
        }
        int port = std::atoi(argv[2]);
        if (port <= 0) {
            std::cerr << "Invalid port\n";
            return 1;
        }
        HttpServer server(engine);
        server.start(port);
        return 0;
    }

    printUsage();
    return 1;
}
