#include "HttpServer.h"
#include "PlotRenderer.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>

HttpServer::HttpServer(int port) : port_(port), serverSocket_(-1), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    running_ = true;

    // Create server socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(serverSocket_);
        return;
    }

    // Bind socket
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        close(serverSocket_);
        return;
    }

    // Listen for connections
    if (listen(serverSocket_, 5) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket_);
        return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    // Main server loop
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }

        // Handle client in a separate thread
        std::thread clientThread(&HttpServer::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        if (serverSocket_ != -1) {
            close(serverSocket_);
            serverSocket_ = -1;
        }
        std::cout << "Server stopped" << std::endl;
    }
}

void HttpServer::handleClient(int clientSocket) {
    try {
        std::string request = readRequest(clientSocket);
        if (request.empty()) {
            close(clientSocket);
            return;
        }

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
        std::cerr << "Error handling client: " << e.what() << std::endl;
        sendResponse(clientSocket, 500, "text/plain", "Internal Server Error");
    }

    close(clientSocket);
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
    size_t firstSpace = request.find(' ');
    if (firstSpace == std::string::npos) {
        throw std::runtime_error("Invalid HTTP request");
    }

    method = request.substr(0, firstSpace);

    size_t secondSpace = request.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos) {
        throw std::runtime_error("Invalid HTTP request");
    }

    path = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);

    // Extract headers
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        throw std::runtime_error("Invalid HTTP request: no header/body separator");
    }

    std::string headerSection = request.substr(0, headerEnd);
    std::istringstream headerStream(headerSection);

    // Skip first line (method/path)
    std::string line;
    std::getline(headerStream, line);

    // Parse headers
    while (std::getline(headerStream, line)) {
        if (line.empty()) continue;

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = Utils::trim(line.substr(0, colonPos));
            std::string value = Utils::trim(line.substr(colonPos + 1));
            headers[key] = value;
        }
    }

    // Extract body
    if (headerEnd + 4 < request.size()) {
        body = request.substr(headerEnd + 4);
    }
}

void HttpServer::handleGetRequest(int clientSocket, const std::string& path) {
    std::string content;
    std::string contentType;

    if (path == "/" || path == "/index.html") {
        content = readFile("public/index.html");
        if (content.empty()) {
            content = readFile("../public/index.html");
        }
        contentType = "text/html";
    } else if (path == "/styles.css") {
        content = readFile("public/styles.css");
        if (content.empty()) {
            content = readFile("../public/styles.css");
        }
        contentType = "text/css";
    } else {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
        return;
    }

    if (content.empty()) {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
    } else {
        sendResponse(clientSocket, 200, contentType, content);
    }
}

void HttpServer::handlePostRequest(int clientSocket, const std::string& path,
                                  const std::string& body) {
    if (path != "/plot") {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
        return;
    }

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

        if (xdata.empty()) {
            sendResponse(clientSocket, 400, "text/plain", "No data provided");
            return;
        }

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

        std::string bmpData = plotImage.toBMP();
        sendResponse(clientSocket, 200, "image/bmp", bmpData);

    } catch (const std::exception& e) {
        std::cerr << "Error processing plot request: " << e.what() << std::endl;
        sendResponse(clientSocket, 400, "text/plain", "Invalid request: " + std::string(e.what()));
    }
}

void HttpServer::sendResponse(int clientSocket, const std::string& response) {
    send(clientSocket, response.c_str(), response.size(), 0);
}

void HttpServer::sendResponse(int clientSocket, int statusCode, const std::string& contentType,
                             const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << getStatusText(statusCode) << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Cache-Control: no-store\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    sendResponse(clientSocket, response.str());
}

std::string HttpServer::getStatusText(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "Unknown Status";
    }
}

std::string HttpServer::readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return "";
    }

    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
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
    } else {
        return "application/octet-stream";
    }
}

std::map<std::string, std::string> HttpServer::parseFormData(const std::string& body) {
    std::map<std::string, std::string> formData;
    std::vector<std::string> pairs = Utils::split(body, '&');

    for (const auto& pair : pairs) {
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos) {
            std::string key = Utils::urlDecode(pair.substr(0, equalPos));
            std::string value = Utils::urlDecode(pair.substr(equalPos + 1));
            formData[key] = value;
        }
    }

    return formData;
}
