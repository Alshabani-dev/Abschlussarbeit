#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "PlotRenderer.h"

#include <string>
#include <unordered_map>

class HttpServer {
public:
    explicit HttpServer(int port);
    void run();

private:
    int port_;
    bool running_;
    PlotRenderer renderer_;

    int createServerSocket() const;
    void handleClient(int clientSocket) const;
    std::string readRequest(int clientSocket) const;
    std::string serveStaticFile(const std::string& path, std::string& contentType) const;
    std::string handlePlotRequest(const std::string& body, std::string& contentType) const;
    std::unordered_map<std::string, std::string> parseFormData(const std::string& body) const;
    std::string buildResponse(const std::string& status,
                              const std::string& contentType,
                              const std::string& body,
                              bool binary) const;
    std::string getContentType(const std::string& path) const;
};

#endif
