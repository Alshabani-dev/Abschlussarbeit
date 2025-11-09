#include "FileHandler.h"

#include <fstream>
#include <stdexcept>
#include <unordered_map>

FileHandler::FileHandler()
    : publicRoot_(std::filesystem::current_path() / "public") {}

void FileHandler::setPublicRoot(const std::filesystem::path& root) {
    publicRoot_ = root;
}

bool FileHandler::serve(const std::string& requestPath, HttpResponse& response) const {
    std::string relativePath = requestPath;
    if (relativePath.empty() || relativePath == "/") {
        relativePath = "index.html";
    } else {
        if (relativePath.front() == '/') {
            relativePath.erase(relativePath.begin());
        }
    }

    if (relativePath.find("..") != std::string::npos) {
        return false;
    }

    const std::filesystem::path filePath = publicRoot_ / relativePath;
    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
        return false;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open static file: " + filePath.string());
    }

    std::vector<char> content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    response.setStatus(200, "OK");
    response.setHeader("Content-Type", guessMimeType(filePath));
    response.setBody(std::move(content));
    return true;
}

std::string FileHandler::guessMimeType(const std::filesystem::path& path) {
    const std::string extension = path.extension().string();
    static const std::unordered_map<std::string, std::string> mimeTypes{
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "application/javascript; charset=utf-8"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".ico", "image/x-icon"},
        {".svg", "image/svg+xml"},
        {".json", "application/json; charset=utf-8"},
        {".txt", "text/plain; charset=utf-8"}};

    if (const auto it = mimeTypes.find(extension); it != mimeTypes.end()) {
        return it->second;
    }
    return "application/octet-stream";
}
