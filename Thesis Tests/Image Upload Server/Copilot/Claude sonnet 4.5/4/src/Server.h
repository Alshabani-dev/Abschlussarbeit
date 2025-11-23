#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>
#include <vector>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileHandler.h"
#include "UploadHandler.h"

class Server {
public:
    Server(int port);
    ~Server();
    
    void run();
    
private:
    int port_;
    int serverSock_;
    FileHandler fileHandler_;
    UploadHandler uploadHandler_;
    
    // Socket operations
    bool setupSocket();
    void handleClient(int clientSock);
    
    // Read request data with buffering
    std::vector<char> readRequest(int clientSock);
    
    // Send response
    void sendResponse(int clientSock, const std::vector<char>& response);
    
    // Routing
    HttpResponse route(const HttpRequest& request);
};

#endif // SERVER_H
