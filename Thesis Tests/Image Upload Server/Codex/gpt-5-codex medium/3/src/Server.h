#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <sys/select.h>
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
    int listenFd_;
    bool running_;

    std::unique_ptr<FileHandler> fileHandler_;
    std::unique_ptr<UploadHandler> uploadHandler_;
    std::unordered_map<int, std::vector<char>> clientBuffers_;

    void setupSocket();
    void eventLoop();
    void acceptNewConnections(fd_set& masterSet, int& maxFd);
    void handleClientData(int clientFd, fd_set& masterSet, int& maxFd);
    void sendResponse(int clientFd, const HttpResponse& response);
    HttpResponse routeRequest(const HttpRequest& request);
    void closeClient(int clientFd, fd_set& masterSet, int& maxFd);
};

#endif  // SERVER_H
