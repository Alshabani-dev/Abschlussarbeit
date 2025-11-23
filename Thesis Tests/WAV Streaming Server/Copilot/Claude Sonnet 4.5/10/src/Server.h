#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include <cerrno>
#include <array>
#include <fstream>
#include <sys/stat.h>

class Server {
public:
    explicit Server(int port);
    ~Server();
    void run();

private:
    int port_;
    int serverSock_;
    bool running_;

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

#endif // SERVER_H
