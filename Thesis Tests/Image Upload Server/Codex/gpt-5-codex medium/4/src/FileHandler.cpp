#include "FileHandler.h"

#include "HttpResponse.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <vector>

FileHandler::FileHandler(std::string publicRoot)
    : publicRoot_(std::move(publicRoot)) {}

bool FileHandler::serve(const std::string& urlPath, HttpResponse& response) {
    const std::string resolved = resolvePath(urlPath);
    if (resolved.empty()) {
        return false;
    }

    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        return false;
    }

    std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    response.setStatus(200, "OK");
    response.setBody(data, detectMimeType(resolved));
    return true;
}

std::string FileHandler::resolvePath(const std::string& urlPath) const {
    std::string relativePath = urlPath;
    if (relativePath.empty() || relativePath == "/") {
        relativePath = "index.html";
    } else {
        if (!relativePath.empty() && relativePath.front() == '/') {
            relativePath.erase(relativePath.begin());
        }
        if (!relativePath.empty() && relativePath.back() == '/') {
            relativePath += "index.html";
        }
    }

    std::filesystem::path root(publicRoot_);
    std::filesystem::path requested(relativePath);
    std::filesystem::path normalized = (root / requested).lexically_normal();

    std::string normalizedRoot = root.lexically_normal().string();
    if (!normalizedRoot.empty() &&
        normalizedRoot.back() != std::filesystem::path::preferred_separator) {
        normalizedRoot.push_back(std::filesystem::path::preferred_separator);
    }
    std::string normalizedTarget = normalized.string();
    if (normalizedTarget.size() < normalizedRoot.size() ||
        normalizedTarget.compare(0, normalizedRoot.size(), normalizedRoot) != 0) {
        return {};
    }

    if (!std::filesystem::exists(normalized) || !std::filesystem::is_regular_file(normalized)) {
        return {};
    }
    return normalized.string();
}

std::string FileHandler::detectMimeType(const std::string& path) {
    static const std::unordered_map<std::string, std::string> mimeTypes{
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "application/javascript"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"}
    };

    std::filesystem::path fsPath(path);
    std::string extension = fsPath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    const auto it = mimeTypes.find(extension);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    return "application/octet-stream";
}
