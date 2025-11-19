#include "HttpServer.h"
#include "PlotRenderer.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

HttpServer::HttpServer(int port) : port_(port), serverSocket_(-1), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to set socket options");
    }

    // Bind socket
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to bind socket to port " + std::to_string(port_));
    }

    // Listen
    if (listen(serverSocket_, 5) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Server listening on port " << port_ << std::endl;
    running_ = true;

    // Main server loop
    while (running_) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error accepting connection" << std::endl;
            }
            continue;
        }

        // Handle client in a new thread would be better, but for simplicity we'll handle sequentially
        handleClient(clientSocket);
        close(clientSocket);
    }
}

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        if (serverSocket_ >= 0) {
            close(serverSocket_);
            serverSocket_ = -1;
        }
        std::cout << "Server stopped" << std::endl;
    }
}

void HttpServer::handleClient(int clientSocket) {
    try {
        std::string request = readRequest(clientSocket);

        std::string method, path;
        std::map<std::string, std::string> headers;
        std::string body;

        parseRequest(request, method, path, headers, body);

        if (method == "GET") {
            handleGetRequest(clientSocket, path);
        } else if (method == "POST") {
            handlePostRequest(clientSocket, path, body);
        } else {
            sendResponse(clientSocket, 405, "text/plain", "Method Not Allowed");
        }
    } catch (const std::exception& e) {
        sendResponse(clientSocket, 500, "text/plain", "Internal Server Error: " + std::string(e.what()));
    }
}

std::string HttpServer::readRequest(int clientSocket) {
    std::string request;
    char buffer[4096];
    ssize_t bytesRead;

    // Read until we get an empty line (end of headers)
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, bytesRead);

        // Check if we have the end of headers
        if (request.find("\r\n\r\n") != std::string::npos) {
            // Check if this is a POST request with Content-Length
            size_t contentLengthPos = request.find("Content-Length:");

            if (contentLengthPos != std::string::npos) {
                // Extract Content-Length value
                size_t start = contentLengthPos + 16; // "Content-Length: " is 16 chars
                size_t end = request.find("\r\n", start);
                if (end != std::string::npos) {
                    std::string lengthStr = request.substr(start, end - start);
                    int contentLength = std::stoi(Utils::trim(lengthStr));

                    // Calculate how much more we need to read
                    size_t headerEnd = request.find("\r\n\r\n") + 4;
                    size_t bodyRead = request.size() - headerEnd;

                    // Keep reading until we have the full body
                    while (bodyRead < static_cast<size_t>(contentLength)) {
                        bytesRead = recv(clientSocket, buffer,
                                        std::min(sizeof(buffer), static_cast<size_t>(contentLength) - bodyRead), 0);
                        if (bytesRead <= 0) break;
                        request.append(buffer, bytesRead);
                        bodyRead += bytesRead;
                    }
                }
            }
            break;
        }
    }

    return request;
}

void HttpServer::parseRequest(const std::string& request, std::string& method,
                            std::string& path, std::map<std::string, std::string>& headers,
                            std::string& body) {
    // Extract method and path from first line
    size_t firstLineEnd = request.find("\r\n");
    if (firstLineEnd == std::string::npos) {
        throw std::runtime_error("Invalid HTTP request");
    }

    std::string firstLine = request.substr(0, firstLineEnd);
    size_t methodEnd = firstLine.find(' ');
    if (methodEnd == std::string::npos) {
        throw std::runtime_error("Invalid HTTP request");
    }

    method = firstLine.substr(0, methodEnd);
    size_t pathEnd = firstLine.find(' ', methodEnd + 1);
    if (pathEnd == std::string::npos) {
        throw std::runtime_error("Invalid HTTP request");
    }

    path = firstLine.substr(methodEnd + 1, pathEnd - methodEnd - 1);

    // Parse headers
    size_t headerStart = firstLineEnd + 2;
    size_t headerEnd = request.find("\r\n\r\n");

    if (headerEnd == std::string::npos) {
        throw std::runtime_error("Invalid HTTP request");
    }

    std::string headersSection = request.substr(headerStart, headerEnd - headerStart);
    std::istringstream headerStream(headersSection);
    std::string headerLine;

    while (std::getline(headerStream, headerLine)) {
        size_t colonPos = headerLine.find(':');
        if (colonPos != std::string::npos) {
            std::string key = Utils::trim(headerLine.substr(0, colonPos));
            std::string value = Utils::trim(headerLine.substr(colonPos + 1));
            headers[key] = value;
        }
    }

    // Extract body
    if (headerEnd + 4 < request.size()) {
        body = request.substr(headerEnd + 4);
    }
}

void HttpServer::handleGetRequest(int clientSocket, const std::string& path) {
    if (path == "/" || path == "/index.html") {
        std::string content = readFile("public/index.html");
        if (!content.empty()) {
            sendResponse(clientSocket, 200, "text/html", content);
        } else {
            sendResponse(clientSocket, 404, "text/plain", "File not found");
        }
    } else if (path == "/styles.css") {
        std::string content = readFile("public/styles.css");
        if (!content.empty()) {
            sendResponse(clientSocket, 200, "text/css", content);
        } else {
            sendResponse(clientSocket, 404, "text/plain", "File not found");
        }
    } else {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
    }
}

void HttpServer::handlePostRequest(int clientSocket, const std::string& path, const std::string& body) {
    if (path == "/plot") {
        try {
            auto formData = parseFormData(body);

            if (formData.find("xdata") == formData.end() ||
                formData.find("ydata") == formData.end() ||
                formData.find("plot") == formData.end()) {
                sendResponse(clientSocket, 400, "text/plain", "Missing required parameters");
                return;
            }

            std::vector<double> xdata = Utils::parseDoubleArray(formData["xdata"]);
            std::vector<double> ydata = Utils::parseDoubleArray(formData["ydata"]);
            std::string plotType = formData["plot"];

            if (xdata.size() != ydata.size()) {
                sendResponse(clientSocket, 400, "text/plain", "X and Y data must have the same length");
                return;
            }

            PlotRenderer renderer;
            Image plotImage(800, 600);

            if (plotType == "line") {
                plotImage = renderer.renderLinePlot(xdata, ydata);
            } else if (plotType == "scatter") {
                plotImage = renderer.renderScatterPlot(xdata, ydata);
            } else if (plotType == "bar") {
                plotImage = renderer.renderBarPlot(xdata, ydata);
            } else {
                sendResponse(clientSocket, 400, "text/plain", "Invalid plot type");
                return;
            }

            std::string bmpData = plotImage.toBMP();
            sendResponse(clientSocket, 200, "image/bmp", bmpData);

        } catch (const std::exception& e) {
            sendResponse(clientSocket, 400, "text/plain", "Invalid data: " + std::string(e.what()));
        }
    } else {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
    }
}

void HttpServer::sendResponse(int clientSocket, int statusCode, const std::string& contentType,
                            const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " ";

    switch (statusCode) {
        case 200: response << "OK"; break;
        case 400: response << "Bad Request"; break;
        case 404: response << "Not Found"; break;
        case 405: response << "Method Not Allowed"; break;
        case 500: response << "Internal Server Error"; break;
        default: response << "Unknown Status"; break;
    }

    response << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Cache-Control: no-store\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";

    std::string header = response.str();
    send(clientSocket, header.c_str(), header.size(), 0);
    send(clientSocket, body.c_str(), body.size(), 0);
}

void HttpServer::sendResponse(int clientSocket, const std::string& response) {
    send(clientSocket, response.c_str(), response.size(), 0);
}

std::string HttpServer::readFile(const std::string& filepath) {
    // Try both public/ and ../public/ paths
    std::vector<std::string> paths = {filepath, "../" + filepath};

    for (const auto& path : paths) {
        std::ifstream file(path, std::ios::binary);
        if (file) {
            std::ostringstream content;
            content << file.rdbuf();
            return content.str();
        }
    }

    return "";
}

std::string HttpServer::getContentType(const std::string& filepath) {
    if (Utils::endsWith(filepath, ".html")) {
        return "text/html";
    } else if (Utils::endsWith(filepath, ".css")) {
        return "text/css";
    } else if (Utils::endsWith(filepath, ".js")) {
        return "application/javascript";
    } else if (Utils::endsWith(filepath, ".bmp")) {
        return "image/bmp";
    }
    return "application/octet-stream";
}

std::map<std::string, std::string> HttpServer::parseFormData(const std::string& body) {
    std::map<std::string, std::string> result;
    std::vector<std::string> pairs = Utils::split(body, '&');

    for (const auto& pair : pairs) {
        size_t equalsPos = pair.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = Utils::urlDecode(pair.substr(0, equalsPos));
            std::string value = Utils::urlDecode(pair.substr(equalsPos + 1));
            result[key] = value;
        }
    }

    return result;
}
