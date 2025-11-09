#include "FileHandler.h"
#include "HttpResponse.h"
#include <fstream>
#include <sys/stat.h>
#include <iostream>

FileHandler::FileHandler(const std::string& rootDir) : rootDir_(rootDir) {}

bool FileHandler::fileExists(const std::string& path) const {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool FileHandler::readFile(const std::string& path, std::vector<char>& content) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read file into vector (binary-safe)
    content.resize(size);
    if (!file.read(content.data(), size)) {
        return false;
    }
    
    file.close();
    return true;
}

bool FileHandler::serve(const std::string& path, std::vector<char>& content, std::string& mimeType) const {
    // Build full file path
    std::string filePath = rootDir_ + path;
    
    // Check if file exists
    if (!fileExists(filePath)) {
        return false;
    }
    
    // Read file content
    if (!readFile(filePath, content)) {
        return false;
    }
    
    // Determine MIME type
    mimeType = HttpResponse::getMimeType(filePath);
    
    return true;
}
