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
    bool isRequestComplete(const std::vector<char>& buffer, size_t& contentLength, size_t& headerEnd);
    void routeRequest(int clientSock, const std::vector<char>& requestData);
};

#endif // SERVER_H
