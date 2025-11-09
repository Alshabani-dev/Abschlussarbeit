#include "FileHandler.h"

#include <fstream>
#include <vector>

namespace {
constexpr const char* NOT_FOUND_BODY = "<html><body><h1>404 Not Found</h1></body></html>";
}

FileHandler::FileHandler(std::string publicDirectory)
    : publicDirectory_(std::move(publicDirectory)) {}

HttpResponse FileHandler::serve(const std::string& requestPath) const {
    const auto resolved = resolvePath(requestPath);
    if (!resolved.has_value()) {
        HttpResponse response(404, "Not Found");
        response.setBody(NOT_FOUND_BODY, "text/html; charset=utf-8");
        return response;
    }

    std::ifstream file(resolved.value(), std::ios::binary);
    if (!file) {
        HttpResponse response(404, "Not Found");
        response.setBody(NOT_FOUND_BODY, "text/html; charset=utf-8");
        return response;
    }

    std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    HttpResponse response(200, "OK");
    response.setBody(data, guessMimeType(resolved.value()));
    return response;
}

std::optional<std::string> FileHandler::resolvePath(const std::string& requestPath) const {
    std::string sanitized = requestPath;
    const auto queryPosition = sanitized.find('?');
    if (queryPosition != std::string::npos) {
        sanitized = sanitized.substr(0, queryPosition);
    }

    if (sanitized.empty() || sanitized == "/") {
        sanitized = "/index.html";
    }

    if (!sanitized.empty() && sanitized.front() == '/') {
        sanitized.erase(sanitized.begin());
    }

    if (sanitized.find("..") != std::string::npos) {
        return std::nullopt;
    }

    return publicDirectory_ + "/" + sanitized;
}

std::string FileHandler::guessMimeType(const std::string& filePath) {
    const auto dotPosition = filePath.find_last_of('.');
    if (dotPosition == std::string::npos) {
        return "application/octet-stream";
    }

    const auto extension = filePath.substr(dotPosition + 1);
    if (extension == "html" || extension == "htm") {
        return "text/html; charset=utf-8";
    }
    if (extension == "css") {
        return "text/css; charset=utf-8";
    }
    if (extension == "js") {
        return "application/javascript";
    }
    if (extension == "jpg" || extension == "jpeg") {
        return "image/jpeg";
    }
    if (extension == "png") {
        return "image/png";
    }
    if (extension == "gif") {
        return "image/gif";
    }
    if (extension == "bmp") {
        return "image/bmp";
    }
    if (extension == "svg") {
        return "image/svg+xml";
    }
    if (extension == "txt") {
        return "text/plain; charset=utf-8";
    }

    return "application/octet-stream";
}
