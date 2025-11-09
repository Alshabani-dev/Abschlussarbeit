#ifndef SERVER_H
#define SERVER_H
#include <string>
#include "HttpRequest.h"
#include "HttpResponse.h"

class Server {
public:
    Server(int port);
    void run();
private:
    int port_;
    void handleClient(int clientSock);
    std::string readRequest(int clientSock);
    void handleGetRequest(const HttpRequest& request, HttpResponse& response);
    void handlePostRequest(const HttpRequest& request, HttpResponse& response);
};
#endif // SERVER_H
