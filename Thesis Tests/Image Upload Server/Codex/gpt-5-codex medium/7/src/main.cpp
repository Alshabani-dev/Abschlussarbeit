#include "Server.h"

#include <csignal>
#include <iostream>
#include <string>

namespace {
int parsePort(int argc, char* argv[], int defaultPort) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            try {
                return std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "Invalid port value: " << argv[i] << std::endl;
                return defaultPort;
            }
        }
    }
    return defaultPort;
}
} // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGPIPE, SIG_IGN);

    const int port = parsePort(argc, argv, 8080);

    try {
        Server server(port, "public", "Data");
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Server failed: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
