#include <iostream>
#include <string>

#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char *argv[]) {
    try {
        Engine engine;
        if (argc > 1) {
            std::string arg1 = argv[1];
            if (arg1 == "--script" && argc > 2) {
                engine.executeScript(argv[2]);
                return 0;
            }
            if (arg1 == "--web" && argc > 2) {
                unsigned short port = static_cast<unsigned short>(std::stoi(argv[2]));
                HttpServer server;
                server.start(port, [&engine](const std::string &sql) {
                    return engine.executeStatementWeb(sql);
                });
                return 0;
            }
            // Treat the argument as a script filename.
            engine.executeScript(arg1);
            return 0;
        }
        engine.repl();
    } catch (const std::exception &ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
