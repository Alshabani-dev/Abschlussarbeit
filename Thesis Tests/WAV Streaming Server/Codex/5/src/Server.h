#pragma once

#include <string>

class Server {
public:
    explicit Server(int port);
    ~Server();

    void run();

private:
    void setupSocket();
    void handleClient(int clientSock);
    void handleGet(const std::string& path, int clientSock);
    void sendFile(int clientSock, const std::string& filePath, const std::string& mimeType);
    void sendAudio(int clientSock);
    void send404(int clientSock);
    void send405(int clientSock);
    void send500(int clientSock, const std::string& message);
    void send400(int clientSock);
    std::string getMimeType(const std::string& path) const;
    bool sendAll(int sock, const char* data, size_t length);

    int port_;
    int serverSock_;
    bool running_;
    const std::string publicDir_;
    const std::string audioFile_;
};
