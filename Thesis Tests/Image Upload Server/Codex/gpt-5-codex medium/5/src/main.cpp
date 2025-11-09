#include "Server.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int parsePort(int argc, char* argv[]) {
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [--port <number>]\n";
            std::exit(EXIT_SUCCESS);
        }
    }
    return port;
}
} // namespace

int main(int argc, char* argv[]) {
    try {
        const int port = parsePort(argc, argv);

        std::filesystem::create_directories("public");
        std::filesystem::create_directories("Data");

        Server server(port, "public", "Data");
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
