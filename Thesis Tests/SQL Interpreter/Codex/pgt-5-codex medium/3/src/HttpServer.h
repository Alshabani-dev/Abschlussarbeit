#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <cstdint>
#include <string>

class Engine;

class HttpServer {
public:
    explicit HttpServer(Engine &engine);
    void start(uint16_t port);

private:
    void handleClient(int clientSocket);
    std::string buildHtmlPage() const;
    std::string urlDecode(const std::string &value) const;
    std::string readRequest(int clientSocket) const;
    std::string extractBody(const std::string &request) const;
    std::string getHeaderValue(const std::string &headers, const std::string &headerName) const;

    Engine &engine_;
};

#endif // HTTP_SERVER_H
