#include "FileHandler.h"
#include "HttpResponse.h"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

FileHandler::FileHandler() : publicDir_("public/") {}

bool FileHandler::serveFile(const std::string& path, int& statusCode,
                           std::string& statusMessage, std::string& contentType,
                           std::vector<char>& body) {
    // Default to index.html for root path
    std::string filePath = path;
    if (filePath == "/" || filePath.empty()) {
        filePath = "/index.html";
    }
    
    // Remove leading slash
    if (!filePath.empty() && filePath[0] == '/') {
        filePath = filePath.substr(1);
    }
    
    // Build full path
    std::string fullPath = publicDir_ + filePath;
    
    // Security: prevent directory traversal
    if (fullPath.find("..") != std::string::npos) {
        statusCode = 403;
        statusMessage = "Forbidden";
        contentType = "text/plain";
        std::string msg = "403 Forbidden";
        body.assign(msg.begin(), msg.end());
        return false;
    }
    
    // Check if file exists
    if (!fileExists(fullPath)) {
        statusCode = 404;
        statusMessage = "Not Found";
        contentType = "text/plain";
        std::string msg = "404 Not Found";
        body.assign(msg.begin(), msg.end());
        return false;
    }
    
    // Read file content
    if (!readFile(fullPath, body)) {
        statusCode = 500;
        statusMessage = "Internal Server Error";
        contentType = "text/plain";
        std::string msg = "500 Internal Server Error";
        body.assign(msg.begin(), msg.end());
        return false;
    }
    
    // Set MIME type
    contentType = HttpResponse::getMimeType(fullPath);
    
    // Success
    statusCode = 200;
    statusMessage = "OK";
    return true;
}

bool FileHandler::readFile(const std::string& fullPath, std::vector<char>& content) {
    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read into vector (binary-safe)
    content.resize(size);
    if (!file.read(content.data(), size)) {
        return false;
    }
    
    return true;
}

bool FileHandler::fileExists(const std::string& fullPath) {
    struct stat buffer;
    return (stat(fullPath.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}
