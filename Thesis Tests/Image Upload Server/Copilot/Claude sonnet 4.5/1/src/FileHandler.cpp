#include "FileHandler.h"
#include <fstream>
#include <sys/stat.h>
#include <algorithm>

FileHandler::FileHandler(const std::string& publicDir) : publicDir_(publicDir) {}

HttpResponse FileHandler::handle(const std::string& path) {
    // Determine the file path
    std::string filepath = publicDir_;
    
    if (path == "/") {
        filepath += "/index.html";
    } else {
        filepath += path;
    }
    
    // Check if file exists
    if (!fileExists(filepath)) {
        return HttpResponse::notFound("File not found: " + path);
    }
    
    // Read file content
    std::vector<char> content = readFile(filepath);
    if (content.empty()) {
        return HttpResponse::internalError("Failed to read file: " + path);
    }
    
    // Get MIME type
    std::string mimeType = getMimeType(filepath);
    
    // Build response
    return HttpResponse::ok(content, mimeType);
}

bool FileHandler::fileExists(const std::string& filepath) const {
    struct stat buffer;
    return (stat(filepath.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

std::vector<char> FileHandler::readFile(const std::string& filepath) const {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
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

std::string FileHandler::getMimeType(const std::string& filepath) const {
    std::string ext = getFileExtension(filepath);
    
    if (ext == "html" || ext == "htm") {
        return "text/html";
    } else if (ext == "css") {
        return "text/css";
    } else if (ext == "js") {
        return "application/javascript";
    } else if (ext == "json") {
        return "application/json";
    } else if (ext == "png") {
        return "image/png";
    } else if (ext == "jpg" || ext == "jpeg") {
        return "image/jpeg";
    } else if (ext == "gif") {
        return "image/gif";
    } else if (ext == "bmp") {
        return "image/bmp";
    } else if (ext == "svg") {
        return "image/svg+xml";
    } else if (ext == "ico") {
        return "image/x-icon";
    } else {
        return "application/octet-stream";
    }
}

std::string FileHandler::getFileExtension(const std::string& filepath) const {
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    
    std::string ext = filepath.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}
