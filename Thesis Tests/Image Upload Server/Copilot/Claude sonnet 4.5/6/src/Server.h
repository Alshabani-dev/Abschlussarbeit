#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <map>

class Server {
public:
    Server(int port);
    ~Server();
    void run();
    
private:
    int port_;
    int serverSocket_;
    
    void setupSocket();
    void handleClient(int clientSock);
    std::vector<char> readRequest(int clientSock);
    void sendResponse(int clientSock, const std::vector<char>& response);
    void setNonBlocking(int sock);
};

#endif // SERVER_H
