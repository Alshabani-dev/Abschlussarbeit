#ifndef SERVER_H
#define SERVER_H
#include <string>
class Server {
public:
    Server(int port);
    void run();
private:
    int port_;
    void handleClient(int clientSock);
    std::string readRequest(int clientSock);
};
#endif // SERVER_H
