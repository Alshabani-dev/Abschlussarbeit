#ifndef SERVER_H
#define SERVER_H

#include "FileHandler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UploadHandler.h"

#include <unordered_map>
#include <vector>

struct pollfd;

class Server {
public:
    explicit Server(int port);
    void run();

private:
    struct ClientConnection {
        std::vector<char> buffer;
        size_t expectedSize = 0;
        bool headerParsed = false;
    };

    int port_;
    int serverFd_;
    FileHandler fileHandler_;
    UploadHandler uploadHandler_;
    std::unordered_map<int, ClientConnection> clients_;

    void setupSocket();
    void eventLoop();
    void acceptConnections(std::vector<pollfd>& pollFds);
    void handleClient(std::vector<pollfd>& pollFds, size_t index);
    bool evaluateRequestBoundary(ClientConnection& client);
    void processBuffer(int clientFd, ClientConnection& client);
    void closeClient(std::vector<pollfd>& pollFds, size_t index);
    void sendResponse(int clientFd, const HttpResponse& response);
    void respondWithStatus(int clientFd, int status, const std::string& reason, const std::string& body);
};

#endif // SERVER_H
