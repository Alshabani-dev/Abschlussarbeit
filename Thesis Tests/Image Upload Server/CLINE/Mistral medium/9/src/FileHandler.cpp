#include "FileHandler.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

std::string FileHandler::handle(const std::string& path) {
    std::string filePath = "public" + path;
    std::ifstream file(filePath, std::ios::binary);

    if (!file) {
        return "404 Not Found";
    }

    // Read file content
    std::ostringstream content;
    content << file.rdbuf();
    std::string fileContent = content.str();

    return fileContent;
}

std::string FileHandler::getMimeType(const std::string& path) {
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

bool FileHandler::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}
