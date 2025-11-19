#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <unordered_map>

class HttpServer {
public:
    explicit HttpServer(int port);
    ~HttpServer();

    void start();
    void stop();

private:
    struct HttpRequest {
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };

    int port_;
    int serverFd_;
    bool running_;

    void handleClient(int clientFd);
    std::string readRequest(int clientFd);
    HttpRequest parseRequest(const std::string &rawRequest);
    std::unordered_map<std::string, std::string> parseFormData(const std::string &body);

    void handleGet(const HttpRequest &request, int clientFd);
    void handlePost(const HttpRequest &request, int clientFd);

    void sendResponse(int clientFd, const std::string &status, const std::string &contentType,
                      const std::string &body, const std::string &extraHeaders = "");
};

#endif
