#include "FileHandler.h"
#include <fstream>
#include <vector>
#include <algorithm>
#include <filesystem>

std::vector<char> FileHandler::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read file content
    std::vector<char> content(size);
    file.read(content.data(), size);

    return content;
}

std::string FileHandler::getMimeType(const std::string& path) {
    std::string extension = path.substr(path.find_last_of(".") + 1);

    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (extension == "html") return "text/html";
    if (extension == "css") return "text/css";
    if (extension == "js") return "application/javascript";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "png") return "image/png";
    if (extension == "gif") return "image/gif";
    if (extension == "bmp") return "image/bmp";

    return "application/octet-stream";
}

bool FileHandler::fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}
