#include "Server.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc != 3 || std::string(argv[1]) != "--port") {
        std::cerr << "Usage: " << argv[0] << " --port <port_number>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[2]);
    Server server(port);

    try {
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
