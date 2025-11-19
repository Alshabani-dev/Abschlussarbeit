#include "HttpServer.h"
#include "Utils.h"
#include "PlotRenderer.h"
#include "Image.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <algorithm>

HttpServer::HttpServer(int port) : port_(port), serverSocket_(-1), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        close(serverSocket_);
        return;
    }

    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket_);
        return;
    }

    running_ = true;
    std::cout << "Server started on port " << port_ << std::endl;

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

        std::thread clientThread(&HttpServer::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        close(serverSocket_);
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

        std::string method, path, body;
        std::map<std::string, std::string> headers;
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

    close(clientSocket);
}

std::string HttpServer::readRequest(int clientSocket) {
    std::string request;
    char buffer[4096];
    ssize_t bytesRead;

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
    std::istringstream iss(request);
    std::string line;

    // Parse request line
    std::getline(iss, line);
    std::istringstream lineStream(line);
    lineStream >> method >> path;

    // Parse headers
    while (std::getline(iss, line) && line != "\r") {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = Utils::trim(line.substr(0, colonPos));
            std::string value = Utils::trim(line.substr(colonPos + 1));
            headers[key] = value;
        }
    }

    // Extract body
    size_t bodyStart = request.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        bodyStart += 4;
        body = request.substr(bodyStart);
    }
}

void HttpServer::handleGetRequest(int clientSocket, const std::string& path) {
    std::string filePath;
    if (path == "/") {
        filePath = "public/index.html";
    } else if (path == "/styles.css") {
        filePath = "public/styles.css";
    } else {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
        return;
    }

    std::string content = readFile(filePath);
    if (content.empty()) {
        content = readFile("../" + filePath);
    }

    if (content.empty()) {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
    } else {
        std::string contentType = getContentType(filePath);
        sendResponse(clientSocket, 200, contentType, content);
    }
}

void HttpServer::handlePostRequest(int clientSocket, const std::string& path, const std::string& body) {
    if (path != "/plot") {
        sendResponse(clientSocket, 404, "text/plain", "Not Found");
        return;
    }

    try {
        auto formData = parseFormData(body);
        std::string xdataStr = formData["xdata"];
        std::string ydataStr = formData["ydata"];
        std::string plotType = formData["plot"];

        std::vector<double> xdata = Utils::parseDoubleArray(xdataStr);
        std::vector<double> ydata = Utils::parseDoubleArray(ydataStr);

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
        sendResponse(clientSocket, 400, "text/plain", "Bad Request: " + std::string(e.what()));
    }
}

void HttpServer::sendResponse(int clientSocket, const std::string& response) {
    send(clientSocket, response.c_str(), response.size(), 0);
}

void HttpServer::sendResponse(int clientSocket, int statusCode, const std::string& contentType, const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode << " ";
    switch (statusCode) {
        case 200: oss << "OK"; break;
        case 400: oss << "Bad Request"; break;
        case 404: oss << "Not Found"; break;
        case 405: oss << "Method Not Allowed"; break;
        case 500: oss << "Internal Server Error"; break;
        default: oss << "Unknown Status"; break;
    }
    oss << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Cache-Control: no-store\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << body;

    sendResponse(clientSocket, oss.str());
}

std::string HttpServer::readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return "";

    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

std::string HttpServer::getContentType(const std::string& filepath) {
    if (Utils::endsWith(filepath, ".html")) return "text/html";
    if (Utils::endsWith(filepath, ".css")) return "text/css";
    if (Utils::endsWith(filepath, ".bmp")) return "image/bmp";
    return "text/plain";
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
