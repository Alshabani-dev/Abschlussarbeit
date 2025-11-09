#include <iostream>
#include "Server.h"

int main(int argc, char* argv[]) {
    if (argc != 3 || std::string(argv[1]) != "--port") {
        std::cerr << "Usage: " << argv[0] << " --port <port_number>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[2]);
    Server server(port);
    server.run();

    return 0;
}
