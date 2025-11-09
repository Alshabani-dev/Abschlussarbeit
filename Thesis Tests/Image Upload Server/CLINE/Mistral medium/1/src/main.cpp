#include "Server.h"
#include <iostream>
#include <csignal>

int main(int argc, char* argv[]) {
    // Ignore SIGPIPE to prevent crashes on client disconnects
    std::signal(SIGPIPE, SIG_IGN);

    if (argc != 3 || std::string(argv[1]) != "--port") {
        std::cerr << "Usage: " << argv[0] << " --port <port_number>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[2]);
    Server server(port);
    server.run();

    return 0;
}
