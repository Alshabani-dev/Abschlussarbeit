#include "FileHandler.h"
#include <fstream>
#include <algorithm>

FileHandler::FileHandler(const std::string& publicDir) : publicDir_(publicDir) {}

bool FileHandler::isSafePath(const std::string& path) const {
    // Prevent directory traversal attacks
    return path.find("..") == std::string::npos;
}

std::string FileHandler::getMimeType(const std::string& filepath) const {
    // Find file extension
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = filepath.substr(dotPos + 1);
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Map extensions to MIME types
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
    if (ext == "pdf") return "application/pdf";
    
    return "application/octet-stream";
}

bool FileHandler::readFile(const std::string& filepath, std::vector<char>& content) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read file contents
    content.resize(size);
    if (!file.read(content.data(), size)) {
        return false;
    }
    
    return true;
}

HttpResponse FileHandler::serveFile(const std::string& path) {
    // Security check
    if (!isSafePath(path)) {
        return HttpResponse::badRequest("Invalid path");
    }
    
    // Build full filepath
    std::string filepath = publicDir_ + path;
    
    // If path is /, serve index.html
    if (path == "/" || path.empty()) {
        filepath = publicDir_ + "/index.html";
    }
    
    // Read file
    std::vector<char> content;
    if (!readFile(filepath, content)) {
        return HttpResponse::notFound("File not found: " + path);
    }
    
    // Determine MIME type
    std::string mimeType = getMimeType(filepath);
    
    // Build response
    HttpResponse response;
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", mimeType);
    response.setBody(content);
    
    return response;
}
