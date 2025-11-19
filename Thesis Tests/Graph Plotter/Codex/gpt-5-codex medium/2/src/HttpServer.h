#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "PlotRenderer.h"
#include "Utils.h"

#include <map>
#include <string>

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
        std::map<std::string, std::string> headers;
        std::string body;
    };

    int port_;
    int serverSocket_;
    bool running_;

    bool readRawRequest(int clientSocket, std::string &rawRequest) const;
    HttpRequest parseRequest(const std::string &raw) const;
    void handleClient(int clientSocket);

    void handleGet(const HttpRequest &request, int clientSocket);
    void handlePost(const HttpRequest &request, int clientSocket);

    void sendTextResponse(int clientSocket, int statusCode, const std::string &statusText,
                          const std::string &contentType, const std::string &body) const;
    void sendBinaryResponse(int clientSocket, int statusCode, const std::string &statusText,
                             const std::string &contentType, const std::string &body) const;

    static std::string getStatusLine(int statusCode, const std::string &statusText);
    static std::string getContentType(const std::string &path);
};

#endif
