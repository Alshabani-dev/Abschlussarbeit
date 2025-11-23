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

HttpServer::HttpServer(int port) 
    : port_(port), serverSocket_(-1), running_(false) {
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(serverSocket_);
        return;
    }
    
    // Bind socket to port
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        close(serverSocket_);
        return;
    }
    
    // Listen for connections
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket_);
        return;
    }
    
    running_ = true;
    std::cout << "Server started on port " << port_ << std::endl;
    std::cout << "Open http://localhost:" << port_ << " in your browser" << std::endl;
    
    // Accept connections
    while (running_) {
        struct sockaddr_in clientAddress;
        socklen_t clientLen = sizeof(clientAddress);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddress, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        handleClient(clientSocket);
        close(clientSocket);
    }
}

void HttpServer::stop() {
    running_ = false;
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
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
    // Find body separator and extract body first
    size_t bodyStart = request.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        bodyStart += 4; // Skip past \r\n\r\n
        body = request.substr(bodyStart);
    }
    
    // Parse request line and headers
    std::istringstream stream(request);
    std::string line;
    
    // Parse request line (GET /path HTTP/1.1)
    if (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        lineStream >> method >> path;
    }
    
    // Parse headers
    while (std::getline(stream, line) && line != "\r") {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            headers[Utils::trim(key)] = Utils::trim(value);
        }
    }
}

void HttpServer::handleClient(int clientSocket) {
    std::string request = readRequest(clientSocket);
    
    if (request.empty()) {
        return;
    }
    
    std::string method, path, body;
    std::map<std::string, std::string> headers;
    parseRequest(request, method, path, headers, body);
    
    std::cout << method << " " << path << std::endl;
    
    if (method == "GET") {
        handleGetRequest(clientSocket, path);
    } else if (method == "POST") {
        handlePostRequest(clientSocket, path, body);
    } else {
        sendResponse(clientSocket, 405, "text/plain", "Method Not Allowed");
    }
}

void HttpServer::handleGetRequest(int clientSocket, const std::string& path) {
    if (path == "/") {
        std::string content = readFile("public/index.html");
        if (content.empty()) {
            content = readFile("../public/index.html");
        }
        
        if (!content.empty()) {
            sendResponse(clientSocket, 200, "text/html", content);
        } else {
            sendResponse(clientSocket, 404, "text/plain", "Not Found");
        }
    } else if (path == "/styles.css") {
        std::string content = readFile("public/styles.css");
        if (content.empty()) {
            content = readFile("../public/styles.css");
        }
        
        if (!content.empty()) {
            sendResponse(clientSocket, 200, "text/css", content);
        } else {
            sendResponse(clientSocket, 404, "text/plain", "Not Found");
        }
    } else {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
    }
}

void HttpServer::handlePostRequest(int clientSocket, const std::string& path, 
                                  const std::string& body) {
    if (path == "/plot") {
        std::map<std::string, std::string> formData = parseFormData(body);
        
        // Extract parameters
        std::string xdataStr = formData["xdata"];
        std::string ydataStr = formData["ydata"];
        std::string plotType = formData["plot"];
        
        // Parse data arrays
        std::vector<double> xdata = Utils::parseDoubleArray(xdataStr);
        std::vector<double> ydata = Utils::parseDoubleArray(ydataStr);
        
        // Validate
        if (xdata.empty() || ydata.empty()) {
            sendResponse(clientSocket, 400, "text/plain", "Error: Empty data arrays");
            return;
        }
        
        if (xdata.size() != ydata.size()) {
            sendResponse(clientSocket, 400, "text/plain", "Error: X and Y data arrays must have equal length");
            return;
        }
        
        // Generate plot
        PlotRenderer renderer(800, 600);
        Image plotImage(800, 600);
        
        if (plotType == "line") {
            plotImage = renderer.renderLinePlot(xdata, ydata);
        } else if (plotType == "scatter") {
            plotImage = renderer.renderScatterPlot(xdata, ydata);
        } else if (plotType == "bar") {
            plotImage = renderer.renderBarPlot(xdata, ydata);
        } else {
            sendResponse(clientSocket, 400, "text/plain", "Error: Invalid plot type");
            return;
        }
        
        // Convert to BMP and send
        std::string bmpData = plotImage.toBMP();
        sendResponse(clientSocket, 200, "image/bmp", bmpData);
    } else {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
    }
}

void HttpServer::sendResponse(int clientSocket, const std::string& response) {
    send(clientSocket, response.c_str(), response.length(), 0);
}

void HttpServer::sendResponse(int clientSocket, int statusCode, const std::string& contentType,
                             const std::string& body) {
    std::string statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        default: statusText = "Unknown"; break;
    }
    
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Cache-Control: no-store\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    std::string responseStr = response.str();
    send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
}

std::string HttpServer::readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

std::string HttpServer::getContentType(const std::string& filepath) {
    if (Utils::endsWith(filepath, ".html")) return "text/html";
    if (Utils::endsWith(filepath, ".css")) return "text/css";
    if (Utils::endsWith(filepath, ".js")) return "application/javascript";
    if (Utils::endsWith(filepath, ".bmp")) return "image/bmp";
    if (Utils::endsWith(filepath, ".png")) return "image/png";
    if (Utils::endsWith(filepath, ".jpg") || Utils::endsWith(filepath, ".jpeg")) return "image/jpeg";
    return "text/plain";
}

std::map<std::string, std::string> HttpServer::parseFormData(const std::string& body) {
    std::map<std::string, std::string> data;
    std::vector<std::string> pairs = Utils::split(body, '&');
    
    for (const std::string& pair : pairs) {
        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos) {
            std::string key = pair.substr(0, eqPos);
            std::string value = pair.substr(eqPos + 1);
            data[Utils::urlDecode(key)] = Utils::urlDecode(value);
        }
    }
    
    return data;
}
