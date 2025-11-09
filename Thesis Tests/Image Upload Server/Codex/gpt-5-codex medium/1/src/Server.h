#ifndef SERVER_H
#define SERVER_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest;
class HttpResponse;
class FileHandler;
class UploadHandler;

class Server {
public:
    explicit Server(int port);
    ~Server();

    void run();
    void stop();

private:
    struct ClientState {
        std::vector<char> buffer;
        bool headersParsed{false};
        std::ptrdiff_t expectedSize{-1};
    };

    int port_;
    int serverSock_{-1};
    bool running_{false};
    std::unique_ptr<FileHandler> fileHandler_;
    std::unique_ptr<UploadHandler> uploadHandler_;
    std::unordered_map<int, ClientState> clients_;

    void setupSocket();
    void eventLoop();
    void acceptClient();
    void handleClientData(int clientSock);
    bool isRequestComplete(ClientState& state);
    void processRequest(int clientSock, ClientState& state);
    HttpResponse routeRequest(const HttpRequest& request);
    void sendResponse(int clientSock, const HttpResponse& response);
    void closeClient(int clientSock);
};

#endif // SERVER_H
