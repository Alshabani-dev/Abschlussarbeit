#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>

class Server {
public:
    Server(int port);
    ~Server();
    
    void run();
    
private:
    int port_;
    int serverSocket_;
    
    void handleClient(int clientSock);
    std::vector<char> readRequest(int clientSock);
    void sendResponse(int clientSock, const std::vector<char>& response);
    bool isRequestComplete(const std::vector<char>& buffer, size_t& contentLength);
};

#endif // SERVER_H
