#ifndef SERVER_H
#define SERVER_H

#include "FileHandler.h"
#include "UploadHandler.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

class Server {
public:
    Server(int port, std::string publicDirectory, std::string uploadDirectory);
    ~Server();

    void run();

private:
    struct ClientState {
        std::vector<char> buffer;
        bool headersParsed = false;
        std::size_t headerEnd = 0;
        std::size_t delimiterLength = 0;
        std::size_t expectedBodyLength = 0;
        std::string method;
    };

    void setupSocket();
    void acceptClient();
    void handleClient(int clientFd);
    bool processClientBuffer(int clientFd, ClientState& state);
    void parseHeadersIfNeeded(ClientState& state);
    HttpResponse routeRequest(const HttpRequest& request);
    void sendResponse(int clientFd, const HttpResponse& response);
    void sendErrorResponse(int clientFd, int statusCode, const std::string& statusMessage, const std::string& body);
    void closeClient(int clientFd);

    int port_;
    int listenFd_;
    std::string publicDirectory_;
    UploadHandler uploadHandler_;
    FileHandler fileHandler_;
    std::unordered_map<int, ClientState> clients_;
};

#endif // SERVER_H
