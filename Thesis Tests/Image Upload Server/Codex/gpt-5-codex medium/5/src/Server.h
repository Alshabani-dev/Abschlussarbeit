#ifndef SERVER_H
#define SERVER_H

#include "FileHandler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UploadHandler.h"

#include <poll.h>
#include <string>
#include <unordered_map>
#include <vector>

class Server {
public:
    Server(int port, std::string publicDir, std::string uploadDir);
    ~Server();

    void run();

private:
    struct ClientState {
        std::vector<char> buffer;
        bool headersComplete{false};
        size_t headerLength{0};
        size_t headerEnd{0};
        size_t expectedBodySize{0};
    };

    int port_;
    int serverSock_{-1};
    std::string publicDir_;
    std::string uploadDir_;
    bool running_{false};

    std::unordered_map<int, ClientState> clients_;
    FileHandler fileHandler_;
    UploadHandler uploadHandler_;

    void setupServerSocket();
    void acceptNewClients();
    void handleClientEvent(int fd);
    void closeClient(int fd);
    void processBuffer(int fd, ClientState& state);
    void dispatchRequest(int fd, ClientState& state, size_t requestSize);
    void sendResponse(int fd, const HttpResponse& response);
};

#endif // SERVER_H
