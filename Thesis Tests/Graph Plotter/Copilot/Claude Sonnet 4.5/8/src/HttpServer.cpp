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
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
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
    std::cout << "Server started on http://localhost:" << port_ << std::endl;
    std::cout << "Open your browser to http://localhost:" << port_ << std::endl;
    
    // Accept connections
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
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
    // Find body separator first (CRITICAL: extract body before line parsing)
    size_t bodyStart = request.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        bodyStart += 4; // Skip past \r\n\r\n
        body = request.substr(bodyStart);
    }
    
    // Parse headers
    std::istringstream stream(request);
    std::string line;
    
    // Parse request line
    if (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        lineStream >> method >> path;
    }
    
    // Parse headers
    while (std::getline(stream, line) && line != "\r") {
        if (line.empty() || line == "\r") break;
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = Utils::trim(line.substr(0, colonPos));
            std::string value = Utils::trim(line.substr(colonPos + 1));
            // Remove trailing \r if present
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            headers[key] = value;
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
    if (path == "/" || path == "/index.html") {
        std::string content = readFile("public/index.html");
        if (content.empty()) {
            content = readFile("../public/index.html");
        }
        
        if (!content.empty()) {
            sendResponse(clientSocket, 200, "text/html", content);
        } else {
            sendResponse(clientSocket, 404, "text/plain", "File not found");
        }
    } else if (path == "/styles.css") {
        std::string content = readFile("public/styles.css");
        if (content.empty()) {
            content = readFile("../public/styles.css");
        }
        
        if (!content.empty()) {
            sendResponse(clientSocket, 200, "text/css", content);
        } else {
            sendResponse(clientSocket, 404, "text/plain", "File not found");
        }
    } else {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
    }
}

void HttpServer::handlePostRequest(int clientSocket, const std::string& path, 
                                   const std::string& body) {
    if (path == "/plot") {
        try {
            // Parse form data
            auto formData = parseFormData(body);
            
            // Extract parameters
            if (formData.find("xdata") == formData.end() || 
                formData.find("ydata") == formData.end() ||
                formData.find("plot") == formData.end()) {
                sendResponse(clientSocket, 400, "text/plain", "Missing required parameters");
                return;
            }
            
            std::string xdataStr = formData["xdata"];
            std::string ydataStr = formData["ydata"];
            std::string plotType = formData["plot"];
            
            // Parse arrays
            std::vector<double> xdata = Utils::parseDoubleArray(xdataStr);
            std::vector<double> ydata = Utils::parseDoubleArray(ydataStr);
            
            // Validate
            if (xdata.empty() || ydata.empty()) {
                sendResponse(clientSocket, 400, "text/plain", "Empty data arrays");
                return;
            }
            
            if (xdata.size() != ydata.size()) {
                sendResponse(clientSocket, 400, "text/plain", "X and Y arrays must have the same length");
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
                sendResponse(clientSocket, 400, "text/plain", "Invalid plot type");
                return;
            }
            
            // Convert to BMP (CRITICAL: use toBMP() not toPPM())
            std::string bmpData = plotImage.toBMP();
            
            // Send response
            sendResponse(clientSocket, 200, "image/bmp", bmpData);
            
        } catch (const std::exception& e) {
            sendResponse(clientSocket, 400, "text/plain", std::string("Error: ") + e.what());
        }
    } else {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
    }
}

void HttpServer::sendResponse(int clientSocket, const std::string& response) {
    send(clientSocket, response.c_str(), response.length(), 0);
}

void HttpServer::sendResponse(int clientSocket, int statusCode, 
                              const std::string& contentType, const std::string& body) {
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
    
    // Send headers
    std::string headers = response.str();
    send(clientSocket, headers.c_str(), headers.length(), 0);
    
    // Send body (binary-safe)
    send(clientSocket, body.c_str(), body.length(), 0);
}

std::string HttpServer::readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

std::string HttpServer::getContentType(const std::string& filepath) {
    if (Utils::endsWith(filepath, ".html")) return "text/html";
    if (Utils::endsWith(filepath, ".css")) return "text/css";
    if (Utils::endsWith(filepath, ".js")) return "application/javascript";
    if (Utils::endsWith(filepath, ".bmp")) return "image/bmp";
    if (Utils::endsWith(filepath, ".ppm")) return "image/x-portable-pixmap";
    return "application/octet-stream";
}

std::map<std::string, std::string> HttpServer::parseFormData(const std::string& body) {
    std::map<std::string, std::string> formData;
    std::vector<std::string> pairs = Utils::split(body, '&');
    
    for (const auto& pair : pairs) {
        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos) {
            std::string key = Utils::urlDecode(pair.substr(0, eqPos));
            std::string value = Utils::urlDecode(pair.substr(eqPos + 1));
            formData[key] = value;
        }
    }
    
    return formData;
}
