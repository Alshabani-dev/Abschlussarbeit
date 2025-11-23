#include "FileHandler.h"
#include <fstream>
#include <sys/stat.h>

FileHandler::FileHandler() {}

bool FileHandler::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::vector<char> FileHandler::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return std::vector<char>();
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        return std::vector<char>();
    }
    
    return buffer;
}

std::string FileHandler::getMimeType(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = path.substr(dotPos + 1);
    
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png") return "image/png";
    if (ext == "gif") return "image/gif";
    if (ext == "bmp") return "image/bmp";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "ico") return "image/x-icon";
    if (ext == "txt") return "text/plain";
    
    return "application/octet-stream";
}

HttpResponse FileHandler::serveFile(const std::string& requestPath) {
    HttpResponse response;
    
    // Build file path relative to public/
    std::string filePath = "public" + requestPath;
    
    // Default to index.html if path is /
    if (requestPath == "/") {
        filePath = "public/index.html";
    }
    
    if (!fileExists(filePath)) {
        response.setStatus(404, "Not Found");
        response.setBody("<html><body><h1>404 Not Found</h1></body></html>");
        response.setHeader("Content-Type", "text/html");
        return response;
    }
    
    std::vector<char> fileContent = readFile(filePath);
    if (fileContent.empty()) {
        response.setStatus(500, "Internal Server Error");
        response.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        response.setHeader("Content-Type", "text/html");
        return response;
    }
    
    response.setStatus(200, "OK");
    response.setBody(fileContent);
    response.setHeader("Content-Type", getMimeType(filePath));
    
    return response;
}
