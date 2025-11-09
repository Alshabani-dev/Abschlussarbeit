#include "Server.h"
#include <iostream>
#include <string>
#include <cstring>

int main(int argc, char* argv[]) {
    int port = 8080;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = std::stoi(argv[i + 1]);
            i++;
        }
    }
    
    try {
        Server server(port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
