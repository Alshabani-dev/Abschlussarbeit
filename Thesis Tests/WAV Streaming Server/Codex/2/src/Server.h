#pragma once

#include <string>

class Server {
public:
    explicit Server(int port);
    ~Server();

    void run();

private:
    int port_;
    int serverSock_;
    bool running_;
    std::string publicDir_;
    std::string dataDir_;

    void setupSocket();
    void handleClient(int clientSock);
    void handleGet(const std::string& path, int clientSock);
    void sendFile(int clientSock, const std::string& filePath, const std::string& mimeType);
    void sendAudio(int clientSock);
    void send404(int clientSock);
    void send500(int clientSock, const std::string& message);
    std::string getMimeType(const std::string& path) const;
    bool sendAll(int sock, const char* data, size_t length);
};
