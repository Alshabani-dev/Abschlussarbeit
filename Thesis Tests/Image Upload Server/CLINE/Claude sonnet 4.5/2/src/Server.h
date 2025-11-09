#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>
#include <vector>
#include "FileHandler.h"
#include "UploadHandler.h"

class Server {
public:
    Server(int port);
    ~Server();
    
    void run();
    
private:
    int port_;
    int server_sock_;
    FileHandler file_handler_;
    UploadHandler upload_handler_;
    
    // Client buffer management
    std::map<int, std::vector<char>> client_buffers_;
    
    void setupSocket();
    void handleClient(int clientSock);
    std::vector<char> readRequest(int clientSock);
    std::vector<char> routeRequest(const class HttpRequest& request);
    void sendResponse(int clientSock, const std::vector<char>& response);
};

#endif // SERVER_H
