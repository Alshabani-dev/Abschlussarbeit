#ifndef SERVER_H
#define SERVER_H

#include <filesystem>
#include <poll.h>
#include <unordered_map>
#include <vector>

#include "FileHandler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UploadHandler.h"

class Server {
public:
    explicit Server(int port);
    ~Server();

    void run();

private:
    int port_;
    int serverSock_;
    bool running_;
    std::filesystem::path projectRoot_;
    std::filesystem::path publicRoot_;
    std::filesystem::path dataRoot_;

    FileHandler fileHandler_;
    UploadHandler uploadHandler_;

    std::unordered_map<int, std::vector<char>> clientBuffers_;

    void setupServerSocket();
    void mainLoop();
    void handleNewConnection(std::vector<pollfd>& pollFds);
    void handleClient(int clientSock);
    void closeClient(int clientSock);
    void sendResponse(int clientSock, const HttpResponse& response);
    HttpResponse routeRequest(const HttpRequest& request);
};

#endif // SERVER_H
