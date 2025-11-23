#include "Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <csignal>
#include <errno.h>

Server::Server(int port) 
    : port_(port), server_sock_(-1), file_handler_("public") {
    // Ignore SIGPIPE to prevent crashes when client disconnects
    std::signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (server_sock_ >= 0) {
        close(server_sock_);
    }
}

void Server::setupSocket() {
    // Create socket
    server_sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // Set socket options (reuse address)
    int opt = 1;
    if (setsockopt(server_sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);
    
    if (bind(server_sock_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket to port " + std::to_string(port_));
    }
    
    // Listen
    if (listen(server_sock_, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
    
    std::cout << "Server listening on port " << port_ << std::endl;
}

void Server::run() {
    setupSocket();
    
    while (true) {
        // Accept client connection
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock_, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_sock < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        // Handle client in the same thread (simple approach)
        // For production, you'd want to use threading or async I/O
        handleClient(client_sock);
    }
}

void Server::handleClient(int clientSock) {
    try {
        // Read request
        std::vector<char> raw_request = readRequest(clientSock);
        
        if (raw_request.empty()) {
            close(clientSock);
            return;
        }
        
        // Parse request
        HttpRequest request;
        if (!request.parse(raw_request)) {
            // Send 400 Bad Request
            HttpResponse response;
            response.setStatusCode(400);
            response.setBody("Bad Request");
            response.setContentType("text/plain");
            sendResponse(clientSock, response.build());
            close(clientSock);
            return;
        }
        
        // Route request
        std::vector<char> response_data = routeRequest(request);
        
        // Send response
        sendResponse(clientSock, response_data);
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
    }
    
    close(clientSock);
}

std::vector<char> Server::readRequest(int clientSock) {
    std::vector<char> buffer;
    char chunk[4096];
    
    while (true) {
        ssize_t bytes_read = recv(clientSock, chunk, sizeof(chunk), 0);
        
        if (bytes_read <= 0) {
            // Connection closed or error
            break;
        }
        
        // Append to buffer
        buffer.insert(buffer.end(), chunk, chunk + bytes_read);
        
        // Try to parse the request to see if it's complete
        HttpRequest temp_request;
        if (temp_request.parse(buffer)) {
            // Request is complete
            break;
        }
        
        // Check if we have headers yet
        std::string buffer_str(buffer.begin(), buffer.end());
        size_t header_end = buffer_str.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            header_end = buffer_str.find("\n\n");
        }
        
        if (header_end != std::string::npos) {
            // We have headers, check Content-Length
            size_t content_length = temp_request.getContentLength();
            
            // For GET requests, we're done
            if (buffer_str.find("GET") == 0 || buffer_str.find("HEAD") == 0) {
                break;
            }
            
            // For POST requests, wait until we have the full body
            size_t header_length = header_end + (buffer_str[header_end] == '\r' ? 4 : 2);
            size_t body_length = buffer.size() - header_length;
            
            if (body_length >= content_length) {
                // We have the full body
                break;
            }
        }
    }
    
    return buffer;
}

std::vector<char> Server::routeRequest(const HttpRequest& request) {
    HttpResponse response;
    
    std::string method = request.getMethod();
    std::string path = request.getPath();
    
    if (method == "GET") {
        // Serve static file
        std::vector<char> file_data = file_handler_.readFile(path);
        
        if (file_data.empty()) {
            // File not found
            response.setStatusCode(404);
            response.setBody("404 Not Found");
            response.setContentType("text/plain");
        } else {
            // File found
            response.setStatusCode(200);
            response.setBody(file_data);
            
            // For MIME type detection, use "index.html" if path is "/" or empty
            std::string mime_path = path;
            if (path == "/" || path.empty()) {
                mime_path = "index.html";
            }
            response.setContentType(HttpResponse::getMimeType(mime_path));
        }
        
    } else if (method == "POST" && path == "/") {
        // Handle file upload
        std::string content_type = request.getHeader("Content-Type");
        std::string result = upload_handler_.handle(request.getBody(), content_type);
        
        // Parse result status
        if (result.find("200") == 0) {
            response.setStatusCode(200);
            response.setBody("File uploaded successfully");
            response.setContentType("text/plain");
        } else if (result.find("400") == 0) {
            response.setStatusCode(400);
            response.setBody(result);
            response.setContentType("text/plain");
        } else {
            response.setStatusCode(500);
            response.setBody(result);
            response.setContentType("text/plain");
        }
        
    } else {
        // Method not allowed
        response.setStatusCode(405);
        response.setBody("Method Not Allowed");
        response.setContentType("text/plain");
    }
    
    return response.build();
}

void Server::sendResponse(int clientSock, const std::vector<char>& response) {
    size_t total_sent = 0;
    size_t response_size = response.size();
    
    while (total_sent < response_size) {
        ssize_t sent = send(clientSock, response.data() + total_sent, 
                           response_size - total_sent, MSG_NOSIGNAL);
        
        if (sent < 0) {
            if (errno == EPIPE || errno == ECONNRESET) {
                // Client disconnected
                std::cerr << "Client disconnected during send" << std::endl;
                break;
            }
            throw std::runtime_error("Failed to send response");
        }
        
        total_sent += sent;
    }
}
