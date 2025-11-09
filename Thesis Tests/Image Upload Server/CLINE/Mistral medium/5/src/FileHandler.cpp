#include "FileHandler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

FileHandler::FileHandler(const std::string& root_dir) : root_dir_(root_dir) {}

FileHandler::~FileHandler() {}

std::vector<char> FileHandler::readFile(const std::string& path) {
    std::string full_path = root_dir_ + sanitizePath(path);

    if (!fileExists(full_path)) {
        throw std::runtime_error("File not found: " + full_path);
    }

    if (isDirectory(full_path)) {
        throw std::runtime_error("Path is a directory: " + full_path);
    }

    std::ifstream file(full_path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + full_path);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Failed to read file: " + full_path);
    }

    return buffer;
}

bool FileHandler::fileExists(const std::string& path) const {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string FileHandler::getMimeType(const std::string& path) const {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";
    }

    std::string ext = path.substr(dot_pos + 1);

    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (ext == "html") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png") return "image/png";
    if (ext == "gif") return "image/gif";
    if (ext == "bmp") return "image/bmp";
    if (ext == "txt") return "text/plain";
    if (ext == "json") return "application/json";

    return "application/octet-stream";
}

std::string FileHandler::getContentType(const std::string& path) const {
    return "Content-Type: " + getMimeType(path);
}

std::string FileHandler::sanitizePath(const std::string& path) const {
    // If path is empty or root, return index.html
    if (path.empty() || path == "/") {
        return "/index.html";
    }

    // If path doesn't start with /, add it
    std::string result = path;
    if (result[0] != '/') {
        result = "/" + result;
    }

    // Remove any path traversal attempts
    std::string sanitized;
    std::istringstream iss(result);
    std::string token;

    while (std::getline(iss, token, '/')) {
        if (token.empty() || token == ".") continue;
        if (token == "..") {
            // Don't allow going up directories
            continue;
        }
        sanitized += "/" + token;
    }

    // If we ended up with an empty path, return index.html
    if (sanitized.empty()) {
        return "/index.html";
    }

    return sanitized;
}

bool FileHandler::isDirectory(const std::string& path) const {
    struct stat stat_buf;
    if (stat(path.c_str(), &stat_buf) != 0) {
        return false;
    }
    return S_ISDIR(stat_buf.st_mode);
}
