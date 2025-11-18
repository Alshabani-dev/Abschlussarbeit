#include <iostream>
#include <string>

#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char *argv[]) {
    Engine engine;

    if (argc == 1) {
        engine.repl();
        return 0;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--web" && argc >= 3) {
        int port = 8080;
        try {
            port = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Invalid port: " << argv[2] << std::endl;
            return 1;
        }
        HttpServer server;
        server.serve(port, [&](const std::string &sql) {
            return engine.executeStatementWeb(sql);
        });
        return 0;
    }

    if (arg1 == "--help") {
        std::cout << "Usage: minisql [script.sql]\n"
                  << "       minisql --web <port>\n";
        return 0;
    }

    // Treat argument as script file.
    engine.executeScript(arg1);
    return 0;
}
