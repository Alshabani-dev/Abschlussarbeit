#include <exception>
#include <iostream>
#include <string>

#include "Engine.h"
#include "HttpServer.h"

namespace {

void printUsage(const std::string &binary) {
    std::cout << "Usage: " << binary << " [script.sql | --web [port]]" << '\n';
    std::cout << "No arguments launches the interactive REPL." << '\n';
}

} // namespace

int main(int argc, char **argv) {
    try {
        Engine engine;
        if (argc == 1) {
            engine.repl();
            return 0;
        }
        std::string arg1 = argv[1];
        if (arg1 == "--help" || arg1 == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg1 == "--web") {
            int port = 8080;
            if (argc >= 3) {
                port = std::stoi(argv[2]);
            }
            HttpServer server(engine);
            server.run(port);
            return 0;
        }
        engine.executeScript(arg1);
    } catch (const std::exception &ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
