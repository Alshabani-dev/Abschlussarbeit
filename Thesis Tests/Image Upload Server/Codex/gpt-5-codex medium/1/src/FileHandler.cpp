#include "FileHandler.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>

#include "HttpResponse.h"

namespace {
bool fileExists(const std::string& path) {
    struct stat buffer {};
    return stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode);
}

std::string extensionOf(const std::string& path) {
    const auto dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return {};
    }
    std::string ext = path.substr(dotPos + 1);
    for (auto& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ext;
}
} // namespace

FileHandler::FileHandler(std::string publicRoot)
    : publicRoot_(std::move(publicRoot)) {}

HttpResponse FileHandler::serve(const std::string& requestPath) const {
    const std::string resolved = resolvePath(requestPath);
    if (!fileExists(resolved)) {
        HttpResponse response;
        response.setStatus(404, "Not Found");
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody("404 Not Found");
        return response;
    }

    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        HttpResponse response;
        response.setStatus(500, "Internal Server Error");
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody("500 Internal Server Error");
        return response;
    }

    std::vector<char> contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    HttpResponse response;
    response.setHeader("Content-Type", mimeTypeForExtension(extensionOf(resolved)));
    response.setBody(contents);
    response.setHeader("Connection", "close");
    return response;
}

std::string FileHandler::resolvePath(const std::string& requestPath) const {
    std::string relative = requestPath;
    if (relative.empty() || relative == "/") {
        relative = "/index.html";
    }

    // prevent directory traversal
    if (relative.find("..") != std::string::npos) {
        return publicRoot_ + "/index.html";
    }

    if (!relative.empty() && relative.front() == '/') {
        relative.erase(relative.begin());
    }
    return publicRoot_ + "/" + relative;
}

std::string FileHandler::mimeTypeForExtension(const std::string& extension) {
    if (extension == "html" || extension == "htm") {
        return "text/html; charset=utf-8";
    }
    if (extension == "css") {
        return "text/css; charset=utf-8";
    }
    if (extension == "js") {
        return "application/javascript; charset=utf-8";
    }
    if (extension == "png") {
        return "image/png";
    }
    if (extension == "jpg" || extension == "jpeg") {
        return "image/jpeg";
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
    return "application/octet-stream";
}
