#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <map>

class HttpServer {
public:
    HttpServer(int port);
    ~HttpServer();

    void start();
    void stop();

private:
    int port_;
    int serverSocket_;
    bool running_;

    void handleClient(int clientSocket);
    std::string readRequest(int clientSocket);
    void parseRequest(const std::string& request, std::string& method,
                     std::string& path, std::map<std::string, std::string>& headers,
                     std::string& body);

    void handleGetRequest(int clientSocket, const std::string& path);
    void handlePostRequest(int clientSocket, const std::string& path, const std::string& body);

    void sendResponse(int clientSocket, const std::string& response);
    void sendResponse(int clientSocket, int statusCode, const std::string& contentType, const std::string& body);

    std::string readFile(const std::string& filepath);
    std::string getContentType(const std::string& filepath);
    std::map<std::string, std::string> parseFormData(const std::string& body);
};

#endif
