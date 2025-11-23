#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>
#include <vector>

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
    std::string getClientIP(int clientSock);
};

#endif // SERVER_H
