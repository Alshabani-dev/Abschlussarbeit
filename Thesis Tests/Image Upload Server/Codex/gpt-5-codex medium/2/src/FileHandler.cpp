#include "FileHandler.h"

#include "HttpResponse.h"

#include <filesystem>
#include <fstream>
#include <iterator>

FileHandler::FileHandler() {
    mimeTypes_ = {
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
}

HttpResponse FileHandler::serveFile(const std::string& root, const std::string& urlPath) {
    HttpResponse response;
    const std::string resolvedPath = resolvePath(root, urlPath);
    if (resolvedPath.empty()) {
        response.setStatus(404, "Not Found");
        response.setHeader("Content-Type", "text/plain");
        response.setBody("404 Not Found");
        return response;
    }

    std::ifstream file(resolvedPath, std::ios::binary);
    if (!file.is_open()) {
        response.setStatus(404, "Not Found");
        response.setHeader("Content-Type", "text/plain");
        response.setBody("404 Not Found");
        return response;
    }

    std::vector<char> body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string extension = std::filesystem::path(resolvedPath).extension().string();
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", detectMimeType(extension));
    response.setBody(body);
    return response;
}

std::string FileHandler::resolvePath(const std::string& root, const std::string& urlPath) const {
    std::string cleanPath = urlPath;
    std::size_t query = cleanPath.find('?');
    if (query != std::string::npos) {
        cleanPath = cleanPath.substr(0, query);
    }

    if (cleanPath.empty() || cleanPath == "/") {
        cleanPath = "/index.html";
    }

    if (cleanPath.front() == '/') {
        cleanPath.erase(cleanPath.begin());
    }

    if (cleanPath.find("..") != std::string::npos) {
        return {};
    }

    std::filesystem::path candidate = (std::filesystem::path(root) / cleanPath).lexically_normal();
    std::filesystem::path rootPath = std::filesystem::absolute(std::filesystem::path(root).lexically_normal());
    if (!std::filesystem::exists(rootPath)) {
        return {};
    }

    std::filesystem::path candidateAbs = std::filesystem::absolute(candidate);
    if (std::mismatch(rootPath.begin(), rootPath.end(), candidateAbs.begin()).first != rootPath.end()) {
        return {};
    }

    return candidateAbs.string();
}

std::string FileHandler::detectMimeType(const std::string& extension) const {
    auto it = mimeTypes_.find(extension);
    if (it != mimeTypes_.end()) {
        return it->second;
    }
    return "application/octet-stream";
}
