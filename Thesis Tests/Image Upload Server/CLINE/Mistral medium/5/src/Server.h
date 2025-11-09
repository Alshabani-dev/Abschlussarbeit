#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <csignal>
#include <memory>

class HttpRequest;
class HttpResponse;
class FileHandler;
class UploadHandler;

class Server {
public:
    Server(int port);
    ~Server();
    void run();

private:
    int port_;
    int server_fd_;
    bool running_;
    std::unique_ptr<FileHandler> file_handler_;
    std::unique_ptr<UploadHandler> upload_handler_;

    void setupSignalHandling();
    void createServerSocket();
    void bindSocket();
    void startListening();
    void handleClient(int client_socket);
    std::string readRequest(int client_socket);
    void sendResponse(int client_socket, const std::string& response);
    void sendBinaryResponse(int client_socket, const std::vector<char>& response, const std::string& content_type);
    void processRequest(int client_socket, const HttpRequest& request);
    static void signalHandler(int signal);
};

#endif // SERVER_H
