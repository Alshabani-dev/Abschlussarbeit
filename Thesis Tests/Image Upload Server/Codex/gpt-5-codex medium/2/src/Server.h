#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <poll.h>
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
    int serverSock_;
    bool running_;

    struct ClientContext {
        std::vector<char> buffer;
        bool headerParsed = false;
        size_t headerEnd = 0;
        size_t contentLength = 0;
        bool expectBody = false;
        std::string method;
    };

    std::unordered_map<int, ClientContext> clients_;

    std::unique_ptr<FileHandler> fileHandler_;
    std::unique_ptr<UploadHandler> uploadHandler_;
    std::vector<pollfd> pollFds_;

    void initSocket();
    void cleanup();
    bool handleClient(int clientSock);
    void closeClient(int clientSock);
    std::vector<char> readRequest(int clientSock, bool& connectionClosed);
    void processRequest(int clientSock, const std::vector<char>& rawRequest);
    void sendResponse(int clientSock, const HttpResponse& response);
};

#endif // SERVER_H
