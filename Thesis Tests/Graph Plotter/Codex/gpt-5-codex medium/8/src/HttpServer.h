#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "PlotRenderer.h"

#include <map>
#include <string>
#include <vector>

class HttpServer {
public:
    explicit HttpServer(int port);

    void run();
    void stop();

private:
    struct HttpRequest {
        std::string method;
        std::string path;
        std::map<std::string, std::string> headers;
        std::string body;
    };

    int port_;
    int serverSocket_;
    bool running_;
    PlotRenderer renderer_;

    std::string readRequest(int clientSocket);
    bool parseRequest(const std::string &raw, HttpRequest &request);
    std::map<std::string, std::string> parseFormData(const std::string &body);

    void handleClient(int clientSocket);
    void handleGetRequest(int clientSocket, const HttpRequest &request);
    void handlePostRequest(int clientSocket, const HttpRequest &request);

    void sendResponse(int clientSocket,
                      const std::string &status,
                      const std::string &contentType,
                      const std::string &body,
                      const std::vector<std::pair<std::string, std::string>> &extraHeaders = {});

    std::string getContentType(const std::string &path) const;
};

#endif
