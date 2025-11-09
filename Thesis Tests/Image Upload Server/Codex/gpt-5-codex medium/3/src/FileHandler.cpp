#include "FileHandler.h"

#include "HttpResponse.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <vector>

namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string detectMimeType(const std::string& extension) {
    static const std::unordered_map<std::string, std::string> kMimeTypes = {
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "application/javascript; charset=utf-8"},
        {".json", "application/json; charset=utf-8"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".txt", "text/plain; charset=utf-8"}
    };

    auto it = kMimeTypes.find(toLower(extension));
    if (it != kMimeTypes.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

std::string sanitizePath(const std::string& rawPath) {
    std::string path = rawPath;

    auto queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        path = path.substr(0, queryPos);
    }

    if (path.empty() || path == "/") {
        return "index.html";
    }

    if (!path.empty() && path.front() == '/') {
        path.erase(path.begin());
    }

    std::replace(path.begin(), path.end(), '\\', '/');

    if (!path.empty() && path.back() == '/') {
        path.append("index.html");
    }

    if (path.find("..") != std::string::npos) {
        return {};
    }

    return path;
}

}  // namespace

FileHandler::FileHandler(const std::string& publicDirectory) : publicDirectory_(publicDirectory) {}

FileHandler::~FileHandler() = default;

bool FileHandler::handle(const std::string& path, HttpResponse& response) const {
    std::string sanitized = sanitizePath(path);
    if (sanitized.empty()) {
        return false;
    }

    std::filesystem::path filePath = std::filesystem::path(publicDirectory_) / sanitized;
    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec) || !std::filesystem::is_regular_file(filePath, ec)) {
        return false;
    }

    std::ifstream input(filePath, std::ios::binary);
    if (!input) {
        return false;
    }

    std::vector<char> contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    std::string mimeType = detectMimeType(filePath.extension().string());

    response.setStatus(200, "OK");
    response.setBody(std::move(contents), mimeType);
    return true;
}
