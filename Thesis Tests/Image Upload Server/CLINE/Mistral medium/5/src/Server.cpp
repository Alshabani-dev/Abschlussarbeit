#include "Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileHandler.h"
#include "UploadHandler.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

Server::Server(int port) : port_(port), server_fd_(-1), running_(true),
    file_handler_(std::make_unique<FileHandler>()),
    upload_handler_(std::make_unique<UploadHandler>()) {
    setupSignalHandling();
}

Server::~Server() {
    if (server_fd_ >= 0) {
        close(server_fd_);
    }
}

void Server::setupSignalHandling() {
    std::signal(SIGPIPE, SIG_IGN);
}

void Server::createServerSocket() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }
}

void Server::bindSocket() {
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr *)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }
}

void Server::startListening() {
    if (listen(server_fd_, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

void Server::run() {
    try {
        createServerSocket();
        bindSocket();
        startListening();

        std::cout << "Server running on port " << port_ << std::endl;

        while (running_) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_fd_, (struct sockaddr *)&client_addr, &client_len);

            if (client_socket < 0) {
                if (running_) {
                    std::cerr << "Error accepting connection" << std::endl;
                }
                continue;
            }

            handleClient(client_socket);
            close(client_socket);
        }
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}

void Server::handleClient(int client_socket) {
    try {
        std::string raw_request = readRequest(client_socket);
        HttpRequest request(raw_request);

        if (!request.isValid()) {
            HttpResponse response(400);
            response.setBody("400 Bad Request: Invalid HTTP request");
            sendResponse(client_socket, response.toString());
            return;
        }

        processRequest(client_socket, request);
    } catch (const std::exception& e) {
        HttpResponse response(500);
        response.setBody(std::string("500 Internal Server Error: ") + e.what());
        sendResponse(client_socket, response.toString());
    }
}

void Server::processRequest(int client_socket, const HttpRequest& request) {
    try {
        std::string method = request.getMethod();
        std::string path = request.getPath();

        if (method == "GET") {
            // Handle GET request for static files
            if (path == "/" || path.empty()) {
                path = "/index.html";
            }

            try {
                std::vector<char> file_data = file_handler_->readFile(path);
                std::string content_type = file_handler_->getContentType(path);

                HttpResponse response(200);
                response.setHeader("Content-Type", content_type);
                response.setBody(file_data);

                sendBinaryResponse(client_socket, response.getBinaryBody(), content_type);
            } catch (const std::exception& e) {
                HttpResponse response(404);
                response.setBody(std::string("404 Not Found: ") + e.what());
                sendResponse(client_socket, response.toString());
            }
        }
        else if (method == "POST") {
            // Handle POST request for file uploads
            std::string content_type = request.getHeader("content-type");

            if (content_type.find("multipart/form-data") != std::string::npos) {
                std::string result = upload_handler_->handleUpload(request.getBody(), content_type);

                HttpResponse response(200);
                // Always return success for uploads, the result message will indicate any errors
                response.setBody(result);
                sendResponse(client_socket, response.toString());
            } else {
                HttpResponse response(400);
                response.setBody("400 Bad Request: Unsupported content type for POST");
                sendResponse(client_socket, response.toString());
            }
        }
        else {
            // Unsupported method
            HttpResponse response(405);
            response.setBody("405 Method Not Allowed");
            sendResponse(client_socket, response.toString());
        }
    } catch (const std::exception& e) {
        HttpResponse response(500);
        response.setBody(std::string("500 Internal Server Error: ") + e.what());
        sendResponse(client_socket, response.toString());
    }
}

void Server::sendBinaryResponse(int client_socket, const std::vector<char>& body_data, const std::string& content_type) {
    // Create a proper HTTP response with headers
    HttpResponse response(200);
    response.setHeader("Content-Type", content_type);
    response.setHeader("Content-Length", std::to_string(body_data.size()));

    // Get the header string
    std::string header_str = response.toString();

    // Find where the headers end and body should begin
    size_t header_end = header_str.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        // If we don't find CRLF, just append our own
        header_str += "\r\n\r\n";
        header_end = header_str.length();
    } else {
        header_end += 4; // Move past the \r\n\r\n
    }

    // Create the full response by combining headers and body
    std::vector<char> full_response;
    full_response.insert(full_response.end(), header_str.begin(), header_str.end());
    full_response.insert(full_response.end(), body_data.begin(), body_data.end());

    // Send the complete response
    ssize_t bytes_sent = send(client_socket, full_response.data(), full_response.size(), MSG_NOSIGNAL);

    if (bytes_sent < 0) {
        std::cerr << "Error sending response: " << strerror(errno) << std::endl;
    } else if (bytes_sent < static_cast<ssize_t>(full_response.size())) {
        std::cerr << "Partial send occurred, only sent " << bytes_sent << " of "
                  << full_response.size() << " bytes" << std::endl;
    }
}

std::string Server::readRequest(int client_socket) {
    const int buffer_size = 4096;
    char buffer[buffer_size];
    std::string request;

    // Read until we get the full request (or timeout)
    while (true) {
        ssize_t bytes_read = recv(client_socket, buffer, buffer_size - 1, 0);
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read] = '\0';
        request += buffer;

        // Check if we have the full request (look for double CRLF)
        if (request.find("\r\n\r\n") != std::string::npos ||
            request.find("\n\n") != std::string::npos) {
            // Check if we have the full body based on Content-Length
            size_t content_length_pos = request.find("Content-Length: ");
            if (content_length_pos != std::string::npos) {
                size_t end_of_line = request.find("\r\n", content_length_pos);
                if (end_of_line == std::string::npos) {
                    end_of_line = request.find("\n", content_length_pos);
                }

                if (end_of_line != std::string::npos) {
                    std::string content_length_str = request.substr(
                        content_length_pos + 16,
                        end_of_line - (content_length_pos + 16));

                    try {
                        size_t content_length = std::stoul(content_length_str);
                        size_t header_end = request.find("\r\n\r\n");
                        if (header_end == std::string::npos) {
                            header_end = request.find("\n\n");
                            if (header_end != std::string::npos) {
                                header_end += 2;
                            }
                        } else {
                            header_end += 4;
                        }

                        if (header_end != std::string::npos &&
                            request.length() >= header_end + content_length) {
                            // We have the full request
                            break;
                        }
                    } catch (...) {
                        // If we can't parse Content-Length, just proceed with what we have
                        break;
                    }
                } else {
                    // No Content-Length found, proceed with what we have
                    break;
                }
            } else {
                // No Content-Length header, proceed with what we have
                break;
            }
        }
    }

    return request;
}

void Server::sendResponse(int client_socket, const std::string& response) {
    send(client_socket, response.c_str(), response.size(), MSG_NOSIGNAL);
}

void Server::signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    // Note: This is a static method, so we can't directly access instance members
    // In a real implementation, we would need a way to access the server instance
}
