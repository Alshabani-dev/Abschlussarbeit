#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <csignal>

class Server {
public:
    Server(int port);
    void run();

private:
    int port_;
    int serverSocket_;
    void handleClient(int clientSock);
    std::string readRequest(int clientSock);
    void ignoreSigpipe();
};
#endif // SERVER_H
