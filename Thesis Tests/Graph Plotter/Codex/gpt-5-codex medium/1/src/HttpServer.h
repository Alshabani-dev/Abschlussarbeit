#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "PlotRenderer.h"

#include <string>
#include <unordered_map>

class HttpServer {
public:
    explicit HttpServer(int port);
    ~HttpServer();

    void run();
    void stop();

private:
    int port_;
    int serverFd_;
    bool running_;

    struct Request {
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };

    Request readRequest(int clientFd);
    void handleClient(int clientFd);
    void handleGet(int clientFd, const Request& request);
    void handlePost(int clientFd, const Request& request);
    void sendResponse(int clientFd, const std::string& status, const std::string& contentType, const std::string& body);
    void sendBinaryResponse(int clientFd, const std::string& status, const std::string& contentType, const std::string& data);

    std::unordered_map<std::string, std::string> parseForm(const std::string& body);
    std::string readFile(const std::string& path);
};

#endif
