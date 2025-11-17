#include <iostream>
#include <string>
#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--web") {
        // Web server mode
        int port = 8080;
        if (argc > 2) {
            port = std::stoi(argv[2]);
        }

        Engine engine;
        HttpServer server(port, engine);

    // We'll handle the routes directly in the HttpServer class
    // since we can't capture the engine reference in a lambda
    // that will be used after the main function exits

        std::cout << "Server running on http://localhost:" << port << std::endl;
        server.start();
    } else if (argc > 1) {
        // Script file mode
        Engine engine;
        engine.executeScript(argv[1]);
    } else {
        // REPL mode
        Engine engine;
        engine.repl();
    }

    return 0;
}
