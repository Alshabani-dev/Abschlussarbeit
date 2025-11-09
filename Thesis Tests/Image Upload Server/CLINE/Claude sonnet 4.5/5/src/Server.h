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
    
    // Client buffers for non-blocking I/O
    std::map<int, std::vector<char>> clientBuffers_;
};

#endif // SERVER_H
