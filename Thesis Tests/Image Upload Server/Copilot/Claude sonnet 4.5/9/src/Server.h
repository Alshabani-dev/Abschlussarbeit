#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <unordered_map>

class Server {
public:
    Server(int port);
    ~Server();
    void run();

private:
    int port_;
    int serverSocket_;
    
    // Client buffer management
    struct ClientBuffer {
        std::vector<char> buffer;
        bool headersParsed = false;
        size_t contentLength = 0;
        size_t headersEndPos = 0;
    };
    
    std::unordered_map<int, ClientBuffer> clientBuffers_;
    
    void setupSocket();
    void handleClient(int clientSock);
    bool readRequest(int clientSock, std::vector<char>& fullRequest);
    void processRequest(int clientSock, const std::vector<char>& requestData);
    std::string getContentType(const std::string& path);
};

#endif // SERVER_H
