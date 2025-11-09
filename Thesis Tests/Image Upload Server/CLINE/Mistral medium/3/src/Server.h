#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class Server {
public:
    Server(int port);
    void run();

private:
    int port_;
    int serverSocket_;
    void handleClient(int clientSock);
    std::string readRequest(int clientSock);
    void sendResponse(int clientSock, const std::string& response);
};
#endif // SERVER_H
