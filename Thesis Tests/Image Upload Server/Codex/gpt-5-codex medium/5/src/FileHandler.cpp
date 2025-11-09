#include "FileHandler.h"

#include <fstream>
#include <iterator>
#include <unordered_map>

FileHandler::FileHandler(std::string rootDir)
    : rootDir_(std::filesystem::absolute(std::move(rootDir)).lexically_normal()) {
    rootDirString_ = rootDir_.string();
    if (!rootDirString_.empty() && rootDirString_.back() != std::filesystem::path::preferred_separator) {
        rootDirString_.push_back(std::filesystem::path::preferred_separator);
    }
}

bool FileHandler::serve(const std::string& requestPath, HttpResponse& response) {
    const auto resolved = resolvePath(requestPath);
    if (resolved.empty() || !std::filesystem::exists(resolved) || !std::filesystem::is_regular_file(resolved)) {
        return false;
    }

    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        return false;
    }

    std::vector<char> contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    response.setStatus(200);
    response.setHeader("Content-Type", detectMimeType(resolved.string()));
    response.setBody(contents);
    return true;
}

std::filesystem::path FileHandler::resolvePath(const std::string& requestPath) const {
    std::string cleanPath = requestPath;
    const auto queryPos = cleanPath.find('?');
    if (queryPos != std::string::npos) {
        cleanPath = cleanPath.substr(0, queryPos);
    }

    if (cleanPath.empty() || cleanPath == "/") {
        cleanPath = "/index.html";
    }

    std::filesystem::path relative = std::filesystem::path(cleanPath).lexically_normal();
    if (relative.is_absolute()) {
        relative = relative.relative_path();
    }

    std::filesystem::path sanitized;
    for (const auto& part : relative) {
        if (part == "." || part == "") {
            continue;
        }
        if (part == "..") {
            return {};
        }
        sanitized /= part;
    }

    if (sanitized.empty()) {
        sanitized = "index.html";
    }

    const auto fullPath = (rootDir_ / sanitized).lexically_normal();
    std::string fullString = fullPath.string();
    if (fullString.compare(0, rootDirString_.size(), rootDirString_) != 0) {
        return {};
    }

    return fullPath;
}

std::string FileHandler::detectMimeType(const std::string& path) {
    static const std::unordered_map<std::string, std::string> mimeMap{
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"}
    };

    const auto ext = std::filesystem::path(path).extension().string();
    auto it = mimeMap.find(ext);
    if (it != mimeMap.end()) {
        return it->second;
    }

    return "application/octet-stream";
}
