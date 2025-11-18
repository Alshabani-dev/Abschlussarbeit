#include <iostream>
#include <string>

#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char **argv) {
    Engine engine;

    if (argc > 1) {
        std::string option = argv[1];
        if (option == "--script" && argc >= 3) {
            engine.executeScript(argv[2]);
            return 0;
        }
        if (option == "--web" && argc >= 3) {
            uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
            HttpServer server(engine);
            server.start(port);
            return 0;
        }
        std::cerr << "Usage: " << argv[0] << " [--script file | --web port]" << std::endl;
        return 1;
    }

    engine.repl();
    return 0;
}
