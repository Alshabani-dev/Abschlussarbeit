#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <atomic>
#include <string>
#include <vector>

#include "PlotRenderer.h"

class HttpServer {
public:
    HttpServer(int port, std::string publicDir);
    ~HttpServer();

    void start();
    void stop();

private:
    int port_;
    std::string publicDir_;
    bool running_;
    int serverFd_;
    PlotRenderer renderer_;

    void handleClient(int clientFd);
    std::string readRequest(int clientFd);
    PlotRenderer::PlotType parsePlotType(const std::string& value) const;
    void handleGet(int clientFd, const std::string& path);
    void handlePostPlot(int clientFd, const std::string& body);
    void sendResponse(int clientFd, const std::string& status,
                      const std::string& contentType,
                      const std::string& body);
    std::string getContentType(const std::string& path) const;
    std::string locatePublicFile(const std::string& requestedPath) const;
};

#endif
