#include "FileHandler.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

std::string FileHandler::getMimeType(const std::string& fileExtension) {
    if (fileExtension == ".html" || fileExtension == ".htm") {
        return "text/html; charset=utf-8";
    } else if (fileExtension == ".css") {
        return "text/css; charset=utf-8";
    } else if (fileExtension == ".js") {
        return "application/javascript; charset=utf-8";
    } else if (fileExtension == ".jpg" || fileExtension == ".jpeg") {
        return "image/jpeg";
    } else if (fileExtension == ".png") {
        return "image/png";
    } else if (fileExtension == ".gif") {
        return "image/gif";
    } else if (fileExtension == ".bmp") {
        return "image/bmp";
    } else {
        return "application/octet-stream";
    }
}

std::string FileHandler::handle(const std::string& path) {
    // Basic security check to prevent directory traversal
    if (path.find("..") != std::string::npos) {
        return "";
    }

    std::string fullPath = "public" + path;
    if (fullPath.back() == '/') {
        fullPath += "index.html";
    }

    // Check if file exists
    struct stat buffer;
    if (stat(fullPath.c_str(), &buffer) != 0) {
        return "";
    }

    // Read file
    std::ifstream file(fullPath, std::ios::binary);
    if (!file) {
        return "";
    }

    // Read file content
    std::ostringstream content;
    content << file.rdbuf();

    return content.str();
}
