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
    std::map<int, std::vector<char>> clientBuffers_;
    
    void setupSocket();
    void handleClient(int clientSock);
    std::vector<char> readRequest(int clientSock);
    void sendResponse(int clientSock, const std::vector<char>& response);
    std::vector<char> routeRequest(const std::string& method, const std::string& path, 
                                    const std::string& contentType, const std::vector<char>& body);
    bool isRequestComplete(const std::vector<char>& buffer);
    size_t getContentLength(const std::vector<char>& buffer);
};

#endif // SERVER_H
