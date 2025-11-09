#include "FileHandler.h"
#include <fstream>
#include <algorithm>

FileHandler::FileHandler() {}

bool FileHandler::readFile(const std::string& path, std::vector<char>& content) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read entire file into vector
    content.resize(size);
    if (!file.read(content.data(), size)) {
        return false;
    }
    
    return true;
}

std::string FileHandler::getMimeType(const std::string& path) const {
    std::string ext = getExtension(path);
    
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

std::string FileHandler::getExtension(const std::string& path) const {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    
    std::string ext = path.substr(dotPos + 1);
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}
