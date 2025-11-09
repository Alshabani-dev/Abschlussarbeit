#include "FileHandler.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

std::string FileHandler::handle(const std::string& path) {
    std::string filePath = "public" + path;

    if (!fileExists(filePath)) {
        return "404 Not Found";
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "500 Internal Server Error";
    }

    // Read file into vector<char> for binary safety
    std::vector<char> content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());

    return std::string(content.begin(), content.end());
}

bool FileHandler::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string FileHandler::getMimeType(const std::string& fileExtension) {
    if (fileExtension == ".html" || fileExtension == ".htm") {
        return "text/html";
    } else if (fileExtension == ".css") {
        return "text/css";
    } else if (fileExtension == ".js") {
        return "application/javascript";
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
