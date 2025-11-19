#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <unordered_map>

class HttpServer {
public:
    HttpServer(int port, std::string publicDir);

    void start();

private:
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
    };

    int port_;
    std::string publicDir_;

    void runLoop(int serverFd);
    void handleClient(int clientFd);

    bool readRawRequest(int clientFd, std::string &raw) const;
    HttpRequest parseRequest(const std::string &raw) const;

    void handleGetRequest(int clientFd, const HttpRequest &request);
    void handlePostRequest(int clientFd, const HttpRequest &request);

    std::string readFile(const std::string &path) const;
    std::string getContentType(const std::string &path) const;

    void sendResponse(int clientFd, const std::string &status,
                      const std::string &contentType, const std::string &body,
                      bool binary = false,
                      const std::string &extraHeaders = "") const;
    void sendError(int clientFd, int statusCode, const std::string &message) const;
};

#endif
