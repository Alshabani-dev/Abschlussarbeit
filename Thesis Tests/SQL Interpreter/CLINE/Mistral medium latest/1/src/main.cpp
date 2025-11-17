#include <iostream>
#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char* argv[]) {
    Engine engine;

    if (argc > 1) {
        std::string arg = argv[1];

        if (arg == "--web") {
            int port = 8080;
            if (argc > 2) {
                try {
                    port = std::stoi(argv[2]);
                } catch (...) {
                    std::cerr << "Invalid port number. Using default 8080." << std::endl;
                }
            }

            HttpServer server(port);
            server.start();
        } else {
            // Assume it's a script file
            engine.executeScript(arg);
        }
    } else {
        // Start REPL
        engine.repl();
    }

    return 0;
}
