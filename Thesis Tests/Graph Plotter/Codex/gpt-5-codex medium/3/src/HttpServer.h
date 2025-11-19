#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <unordered_map>

class HttpServer {
public:
    explicit HttpServer(int port);
    void run();

private:
    struct HttpRequest {
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };

    int port_;

    void handleClient(int clientSocket) const;
    std::string readRequest(int clientSocket) const;
    bool parseRequest(const std::string& raw, HttpRequest& request) const;
    void handleGetRequest(int clientSocket, const HttpRequest& request) const;
    void handlePostRequest(int clientSocket, const HttpRequest& request) const;
    void sendResponse(int clientSocket, const std::string& status,
                      const std::string& contentType, const std::string& body) const;
    std::string getContentType(const std::string& path) const;
};

#endif
