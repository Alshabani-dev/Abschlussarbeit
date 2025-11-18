#include <iostream>
#include <string>
#include <thread>

#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char **argv) {
    Engine engine;
    if (argc == 1) {
        engine.repl();
        return 0;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--web") {
        unsigned short port = 8080;
        if (argc >= 3) {
            port = static_cast<unsigned short>(std::stoi(argv[2]));
        }
        HttpServer server;
        server.start(port, [&engine](const std::string &sql) {
            return engine.executeStatementWeb(sql);
        });
        return 0;
    }

    engine.executeScript(arg1);
    return 0;
}
