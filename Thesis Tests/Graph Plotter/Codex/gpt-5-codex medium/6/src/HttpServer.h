#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "PlotRenderer.h"

#include <string>

class HttpServer {
public:
    explicit HttpServer(int port);
    void start();

private:
    int port_;
    int serverFd_;
    PlotRenderer renderer_;

    std::string readRequest(int clientFd);
    void handleClient(int clientFd);
    void handleGetRequest(int clientFd, const std::string& path);
    void handlePostRequest(int clientFd, const std::string& body);
    void sendResponse(int clientFd, const std::string& status,
                      const std::string& contentType, const std::string& body);
    std::string getContentType(const std::string& path) const;
};

#endif
