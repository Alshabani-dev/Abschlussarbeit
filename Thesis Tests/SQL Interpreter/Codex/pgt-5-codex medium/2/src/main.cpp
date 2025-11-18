#include <exception>
#include <iostream>
#include <string>

#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char **argv) {
    try {
        Engine engine;
        if (argc == 1) {
            engine.repl();
        } else {
            std::string arg1 = argv[1];
            if (arg1 == "--web") {
                int port = 8080;
                if (argc >= 3) {
                    port = std::stoi(argv[2]);
                }
                HttpServer server(engine);
                server.run(port);
            } else {
                engine.executeScript(arg1);
            }
        }
    } catch (const std::exception &ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
