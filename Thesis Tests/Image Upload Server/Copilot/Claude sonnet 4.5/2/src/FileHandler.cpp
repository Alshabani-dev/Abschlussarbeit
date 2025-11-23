#include "FileHandler.h"
#include <fstream>
#include <algorithm>
#include <sys/stat.h>

FileHandler::FileHandler(const std::string& public_dir) : public_dir_(public_dir) {}

std::vector<char> FileHandler::readFile(const std::string& path) {
    std::string sanitized_path = sanitizePath(path);
    std::string full_path = public_dir_ + "/" + sanitized_path;
    
    // Open file in binary mode
    std::ifstream file(full_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return std::vector<char>();
    }
    
    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read file into vector (binary-safe)
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        return std::vector<char>();
    }
    
    return buffer;
}

bool FileHandler::fileExists(const std::string& path) {
    std::string sanitized_path = sanitizePath(path);
    std::string full_path = public_dir_ + "/" + sanitized_path;
    
    struct stat buffer;
    return (stat(full_path.c_str(), &buffer) == 0);
}

std::string FileHandler::sanitizePath(const std::string& path) {
    std::string sanitized = path;
    
    // Remove leading slash if present
    if (!sanitized.empty() && sanitized[0] == '/') {
        sanitized = sanitized.substr(1);
    }
    
    // Remove any ".." to prevent directory traversal
    size_t pos = 0;
    while ((pos = sanitized.find("..", pos)) != std::string::npos) {
        sanitized.erase(pos, 2);
    }
    
    // If path is empty or just "/", default to index.html
    if (sanitized.empty() || sanitized == "/") {
        sanitized = "index.html";
    }
    
    return sanitized;
}
