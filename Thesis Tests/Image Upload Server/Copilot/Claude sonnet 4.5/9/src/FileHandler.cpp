#include "FileHandler.h"
#include <fstream>
#include <sys/stat.h>

FileHandler::FileHandler() {}

bool FileHandler::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool FileHandler::readFile(const std::string& path, std::vector<char>& content) {
    if (!fileExists(path)) {
        return false;
    }
    
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    content.resize(size);
    if (!file.read(content.data(), size)) {
        return false;
    }
    
    file.close();
    return true;
}

bool FileHandler::writeFile(const std::string& path, const std::vector<char>& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(content.data(), content.size());
    file.close();
    
    return true;
}
