#ifndef SERVER_H
#define SERVER_H

#include "FileHandler.h"
#include "UploadHandler.h"

#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest;
class HttpResponse;

class Server {
public:
    explicit Server(int port);
    void run();

private:
    int port_;
    int serverSock_;
    FileHandler fileHandler_;
    UploadHandler uploadHandler_;
    std::unordered_map<int, std::vector<char>> buffers_;

    void setupSocket();
    void eventLoop();
    void closeClient(int clientSock, fd_set& masterSet);
    void processRequest(int clientSock, const HttpRequest& request);
    void respond(int clientSock, const HttpResponse& response);
};

#endif // SERVER_H
