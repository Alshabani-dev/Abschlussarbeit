#include "Engine.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    Engine engine;

    // Check for command line arguments
    if (argc > 1) {
        std::string arg = argv[1];

        // Check for web mode
        if (arg == "--web") {
            int port = 8080;
            if (argc > 2) {
                try {
                    port = std::stoi(argv[2]);
                } catch (...) {
                    std::cerr << "Invalid port number. Using default 8080." << std::endl;
                }
            }
            std::cout << "Web server mode not yet implemented. Using REPL instead." << std::endl;
            engine.repl();
        }
        // Check for script file
        else {
            engine.executeScript(arg);
        }
    }
    // Default to REPL mode
    else {
        engine.repl();
    }

    return 0;
}
