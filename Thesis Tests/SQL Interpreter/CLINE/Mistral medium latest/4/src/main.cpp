#include <iostream>
#include <string>
#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char* argv[]) {
    // Check for web mode
    bool webMode = false;
    int port = 8080;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--web") {
            webMode = true;
            if (i + 1 < argc) {
                try {
                    port = std::stoi(argv[i+1]);
                    i++; // Skip the port number
                } catch (...) {
                    // Use default port if conversion fails
                }
            }
        }
    }

    if (webMode) {
        Engine engine;
        HttpServer server(port, [&engine](const std::string& sql) {
            return engine.executeStatementWeb(sql);
        });

        std::cout << "Starting web server on port " << port << std::endl;
        server.start();
    } else if (argc > 1) {
        // Script mode
        Engine engine;
        engine.executeScript(argv[1]);
    } else {
        // REPL mode
        Engine engine;
        engine.repl();
    }

    return 0;
}
