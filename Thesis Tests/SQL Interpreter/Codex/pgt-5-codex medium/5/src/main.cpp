#include <exception>
#include <iostream>
#include <string>

#include "Engine.h"
#include "HttpServer.h"

int main(int argc, char **argv) {
    try {
        Engine engine;
        if (argc > 1) {
            std::string arg1 = argv[1];
            if (arg1 == "--script" && argc >= 3) {
                engine.executeScript(argv[2]);
                return 0;
            }
            if (arg1 == "--web" && argc >= 3) {
                int port = std::stoi(argv[2]);
                HttpServer server(port);
                server.start([&engine](const std::string &sql) {
                    return engine.executeStatementWeb(sql);
                });
                return 0;
            }
            if (arg1 == "--help" || arg1 == "-h") {
                std::cout << "Usage: minisql [--script file | --web port]" << std::endl;
                return 0;
            }
            std::cerr << "Unknown argument. Use --help for usage." << std::endl;
            return 1;
        }
        engine.repl();
    } catch (const std::exception &ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
