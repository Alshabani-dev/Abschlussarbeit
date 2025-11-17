#include <iostream>
#include <string>
#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--web") {
        int port = 8080;
        if (argc > 2) {
            port = std::stoi(argv[2]);
        }

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
