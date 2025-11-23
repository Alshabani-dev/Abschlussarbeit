#include "FileHandler.h"
#include <fstream>
#include <sstream>
#include <algorithm>

FileHandler::FileHandler() {
}

std::string FileHandler::serveFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    
    if (!file.is_open()) {
        return "404 Not Found: File does not exist";
    }
    
    // Read entire file into string
    std::ostringstream fileStream;
    fileStream << file.rdbuf();
    
    return fileStream.str();
}

std::string FileHandler::getFileExtension(const std::string& filepath) const {
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos != std::string::npos && dotPos < filepath.length() - 1) {
        std::string ext = filepath.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    return "";
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
        return "text/plain";
    }
}
