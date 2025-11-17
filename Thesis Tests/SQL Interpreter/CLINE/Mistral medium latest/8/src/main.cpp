#include <iostream>
#include "Engine.h"
#include "HttpServer.h"
#include <string>

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

            HttpServer server(port, engine);
            std::cout << "Starting web server on port " << port << std::endl;
            server.start();
        } else {
            // Execute script file
            engine.executeScript(arg);
        }
    } else {
        // Start REPL
        engine.repl();
    }

    return 0;
}
