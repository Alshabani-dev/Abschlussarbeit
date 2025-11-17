#include <iostream>
#include <string>
#include <thread>
#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char* argv[]) {
    Engine engine;

    // Check for command line arguments
    if (argc > 1) {
        std::string arg = argv[1];

        // Web server mode
        if (arg == "--web") {
            int port = 8080;
            if (argc > 2) {
                try {
                    port = std::stoi(argv[2]);
                } catch (...) {
                    std::cerr << "Invalid port number. Using default 8080." << std::endl;
                }
            }

            // Create and start HTTP server
            HttpServer server(port, [&engine](const std::string& sql) {
                return engine.executeStatementWeb(sql);
            });

            std::thread serverThread([&server]() {
                server.start();
            });

            std::cout << "Web server started on port " << port << std::endl;
            std::cout << "Open your browser to http://localhost:" << port << std::endl;
            std::cout << "Press Enter to stop the server..." << std::endl;

            // Wait for user input to stop the server
            std::string input;
            std::getline(std::cin, input);

            // Stop the server
            server.stop();
            serverThread.join();
        }
        // Script file mode
        else {
            engine.executeScript(arg);
        }
    }
    // Default REPL mode
    else {
        engine.repl();
    }

    return 0;
}
