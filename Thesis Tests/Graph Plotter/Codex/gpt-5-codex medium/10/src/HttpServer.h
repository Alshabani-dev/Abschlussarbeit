#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "PlotRenderer.h"

#include <map>
#include <string>

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpServer {
public:
    HttpServer(int port, const std::string &publicDir);
    void run();
    void stop();

private:
    int port_;
    std::string publicDir_;
    PlotRenderer renderer_;
    int serverFd_;
    bool running_;

    int createServerSocket();
    void handleClient(int clientFd);
    std::string readRequest(int clientFd);
    bool parseRequest(const std::string &raw, HttpRequest &request);
    std::map<std::string, std::string> parseFormData(const std::string &body);

    void handleGet(int clientFd, const HttpRequest &request);
    void handlePost(int clientFd, const HttpRequest &request);

    std::string getContentType(const std::string &path) const;
    void sendResponse(int clientFd, const std::string &status,
                      const std::map<std::string, std::string> &headers,
                      const std::string &body);
};

#endif
