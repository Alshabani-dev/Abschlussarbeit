#include <iostream>
#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char* argv[]) {
    Engine engine;

    // Check if we have command line arguments
    if (argc > 1) {
        // Check for web mode
        if (std::string(argv[1]) == "--web") {
            int port = 8080; // Default port
            if (argc > 2) {
                try {
                    port = std::stoi(argv[2]);
                } catch (...) {
                    std::cerr << "Invalid port number. Using default port 8080." << std::endl;
                }
            }

            // Create HTTP server with SQL executor
            HttpServer server(port, [&engine](const std::string& sql) {
                return engine.executeStatementWeb(sql);
            });

            // Start the server
            server.start();

            return 0;
        }
        // Otherwise, treat as script file
        else {
            engine.executeScript(argv[1]);
            return 0;
        }
    }

    // Default to REPL mode
    engine.repl();
    return 0;
}
