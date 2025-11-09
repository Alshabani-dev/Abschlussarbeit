#include "FileHandler.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

std::string FileHandler::handle(const std::string& path) {
    // Security check: prevent directory traversal
    if (path.find("..") != std::string::npos) {
        return "";
    }

    // Default to index.html if path is root
    std::string filePath = "public";
    if (path == "/") {
        filePath += "/index.html";
    } else {
        filePath += path;
    }

    // Check if file exists
    struct stat buffer;
    if (stat(filePath.c_str(), &buffer) != 0) {
        return "";
    }

    // Read file content
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "";
    }

    // Read file into vector<char> for binary safety
    std::vector<char> content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());

    return std::string(content.begin(), content.end());
}

std::string FileHandler::getMimeType(const std::string& path) {
    // Special case for root path which serves index.html
    if (path == "/") {
        return "text/html";
    }

    if (path.find(".html") != std::string::npos) {
        return "text/html";
    } else if (path.find(".css") != std::string::npos) {
        return "text/css";
    } else if (path.find(".js") != std::string::npos) {
        return "application/javascript";
    } else if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos) {
        return "image/jpeg";
    } else if (path.find(".png") != std::string::npos) {
        return "image/png";
    } else if (path.find(".gif") != std::string::npos) {
        return "image/gif";
    } else if (path.find(".bmp") != std::string::npos) {
        return "image/bmp";
    }

    return "application/octet-stream";
}
