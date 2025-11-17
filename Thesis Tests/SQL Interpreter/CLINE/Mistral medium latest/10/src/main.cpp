#include "Engine.h"
#include "HttpServer.h"
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char* argv[]) {
    Engine engine;

    if (argc == 1) {
        // Interactive mode
        engine.repl();
    } else if (argc == 2 && std::string(argv[1]) == "--web") {
        // Web server mode with default port
        HttpServer server(8080, engine);
        std::cout << "Starting web server on port 8080...\n";
        server.start();
    } else if (argc == 3 && std::string(argv[1]) == "--web") {
        // Web server mode with custom port
        int port = std::stoi(argv[2]);
        HttpServer server(port, engine);
        std::cout << "Starting web server on port " << port << "...\n";
        server.start();
    } else {
        // Script mode
        engine.executeScript(argv[1]);
    }

    return 0;
}
