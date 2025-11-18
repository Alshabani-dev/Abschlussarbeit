#include <iostream>
#include <string>

#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char **argv) {
    Engine engine;
    std::string scriptFile;
    bool webMode = false;
    int webPort = 8080;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--script" && i + 1 < argc) {
            scriptFile = argv[++i];
        } else if (arg == "--web" && i + 1 < argc) {
            webMode = true;
            webPort = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: minisql [--script file] [--web port]" << std::endl;
            return 0;
        } else {
            scriptFile = arg;
        }
    }

    if (!scriptFile.empty() && !webMode) {
        engine.executeScript(scriptFile);
        return 0;
    }

    if (webMode) {
        HttpServer server(engine, webPort);
        server.start();
        return 0;
    }

    engine.repl();
    return 0;
}
