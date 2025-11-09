#include "FileHandler.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

FileHandler::FileHandler(std::string rootDirectory)
    : rootDirectory_(std::move(rootDirectory)) {}

bool FileHandler::readFile(const std::string& requestPath, std::vector<char>& data, std::string& mimeType) const {
    const std::string sanitized = sanitize(requestPath);
    if (sanitized.empty()) {
        return false;
    }

    const fs::path fullPath = fs::path(rootDirectory_) / sanitized;
    std::error_code ec;
    if (!fs::exists(fullPath, ec) || !fs::is_regular_file(fullPath, ec)) {
        return false;
    }

    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    mimeType = guessMime(fullPath.string());
    return true;
}

std::string FileHandler::sanitize(const std::string& requestPath) {
    if (requestPath.empty() || requestPath[0] != '/') {
        return {};
    }

    std::string cleaned = requestPath;
    const auto queryPos = cleaned.find('?');
    if (queryPos != std::string::npos) {
        cleaned = cleaned.substr(0, queryPos);
    }

    if (cleaned == "/") {
        return "index.html";
    }

    if (cleaned.find("..") != std::string::npos) {
        return {};
    }

    if (cleaned.front() == '/') {
        cleaned.erase(cleaned.begin());
    }

    return cleaned;
}

std::string FileHandler::guessMime(const std::string& path) {
    const fs::path ext = fs::path(path).extension();
    const std::string extension = ext.string();

    if (extension == ".html" || extension == ".htm") {
        return "text/html; charset=utf-8";
    }
    if (extension == ".css") {
        return "text/css; charset=utf-8";
    }
    if (extension == ".js") {
        return "application/javascript; charset=utf-8";
    }
    if (extension == ".jpg" || extension == ".jpeg") {
        return "image/jpeg";
    }
    if (extension == ".png") {
        return "image/png";
    }
    if (extension == ".gif") {
        return "image/gif";
    }
    if (extension == ".bmp") {
        return "image/bmp";
    }
    if (extension == ".svg") {
        return "image/svg+xml";
    }
    return "application/octet-stream";
}
