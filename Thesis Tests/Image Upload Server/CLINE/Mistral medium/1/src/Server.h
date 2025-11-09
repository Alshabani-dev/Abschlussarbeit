#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <vector>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileHandler.h"
#include "UploadHandler.h"

class Server {
public:
    Server(int port);
    void run();
private:
    int port_;
    FileHandler fileHandler_;
    UploadHandler uploadHandler_;

    void handleClient(int clientSock);
    std::vector<char> readRequest(int clientSock);
    HttpResponse processRequest(const HttpRequest& request);
};
#endif // SERVER_H
