#ifndef SERVER_H
#define SERVER_H

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

private:
    int port_;
    std::string publicDir_;
    std::string uploadDir_;

    struct ClientState {
        std::vector<char> buffer;
        bool closed{false};
    };

    std::unique_ptr<FileHandler> fileHandler_;
    std::unique_ptr<UploadHandler> uploadHandler_;

    void setupSignalHandling() const;
    int createListeningSocket() const;
    void eventLoop(int listenSock);
    void acceptClients(int listenSock, std::unordered_map<int, ClientState>& clients);
    void handleClientData(int clientSock, ClientState& state);
    void processRequest(int clientSock, ClientState& state);
    void sendResponse(int clientSock, const HttpResponse& response);
};

#endif // SERVER_H
