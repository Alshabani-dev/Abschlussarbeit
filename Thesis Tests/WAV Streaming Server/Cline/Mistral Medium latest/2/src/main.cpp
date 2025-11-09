#include <cstdlib>
#include <cstring>
#include "Server.h"

int main(int argc, char* argv[]) {
    int port = 8080;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            break;
        }
    }

    Server server(port);
    server.run();
    return 0;
}
