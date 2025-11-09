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
    int serverSock_;
    
    void handleClient(int clientSock);
    std::vector<char> readRequest(int clientSock);
    void sendResponse(int clientSock, const std::vector<char>& response);
    std::string getMimeType(const std::string& path);
};

#endif // SERVER_H
