#ifndef SERVER_H
#define SERVER_H

#include <memory>
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
    struct ClientContext {
        std::vector<char> buffer;
        bool headersParsed = false;
        size_t expectedContentLength = 0;
        size_t headerLength = 0;
        size_t delimiterLength = 0;
        bool shouldKeepAlive = false;
    };

    int port_;
    int serverSocket_;
    bool running_;

    std::unique_ptr<FileHandler> fileHandler_;
    std::unique_ptr<UploadHandler> uploadHandler_;
    std::unordered_map<int, ClientContext> clients_;

    void setupListeningSocket();
    void acceptConnections();
    bool handleClient(int clientSocket, ClientContext& context);
    void closeClient(int clientSocket);
    void processRequest(int clientSocket, ClientContext& context);
    void sendResponse(int clientSocket, const HttpResponse& response);
    void configureSignals();
};

#endif // SERVER_H
