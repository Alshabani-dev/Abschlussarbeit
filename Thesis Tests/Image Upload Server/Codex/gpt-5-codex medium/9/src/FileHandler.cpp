#include "FileHandler.h"

#include "HttpResponse.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

std::string ltrimSlashes(const std::string& input) {
    size_t index = 0;
    while (index < input.size() && input[index] == '/') {
        ++index;
    }
    return input.substr(index);
}

bool containsTraversal(const std::string& path) {
    if (path.find("..") == std::string::npos) {
        return false;
    }
    std::stringstream ss(path);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (segment == "..") {
            return true;
        }
    }
    return false;
}

} // namespace

FileHandler::FileHandler(std::string baseDir) : baseDir_(std::move(baseDir)) {}

bool FileHandler::serveFile(const std::string& path, HttpResponse& response) const {
    const std::string resolved = resolvePath(baseDir_, path);
    if (resolved.empty()) {
        return false;
    }

    std::vector<char> contents;
    if (!loadFile(resolved, contents)) {
        return false;
    }

    response.setStatus(200, "OK");
    response.setHeader("Content-Type", detectMimeType(resolved));
    response.setBody(std::move(contents));
    return true;
}

std::string FileHandler::resolvePath(const std::string& baseDir, const std::string& path) {
    if (path.empty()) {
        return {};
    }

    const std::string sanitized = ltrimSlashes(path);
    if (sanitized.empty() || containsTraversal(sanitized)) {
        return {};
    }

    std::filesystem::path fullPath = std::filesystem::path(baseDir) / sanitized;
    std::error_code ec;
    std::filesystem::path canonicalBase = std::filesystem::weakly_canonical(baseDir, ec);
    if (ec) {
        return {};
    }
    std::filesystem::path canonicalTarget = std::filesystem::weakly_canonical(fullPath, ec);
    if (ec) {
        return {};
    }

    const auto canonicalBaseStr = canonicalBase.generic_string();
    const auto canonicalTargetStr = canonicalTarget.generic_string();
    if (canonicalTargetStr.size() < canonicalBaseStr.size() ||
        canonicalTargetStr.compare(0, canonicalBaseStr.size(), canonicalBaseStr) != 0) {
        return {};
    }

    return canonicalTargetStr;
}

bool FileHandler::loadFile(const std::string& filePath, std::vector<char>& buffer) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return false;
    }
    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    if (size < 0) {
        return false;
    }
    file.seekg(0, std::ios::beg);
    buffer.resize(static_cast<size_t>(size));
    if (size > 0) {
        file.read(buffer.data(), size);
        if (!file) {
            return false;
        }
    }
    return true;
}

std::string FileHandler::detectMimeType(const std::string& filePath) {
    const auto pos = filePath.find_last_of('.');
    if (pos == std::string::npos) {
        return "application/octet-stream";
    }
    std::string ext = filePath.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (ext == "html" || ext == "htm") {
        return "text/html; charset=utf-8";
    }
    if (ext == "css") {
        return "text/css; charset=utf-8";
    }
    if (ext == "js") {
        return "application/javascript; charset=utf-8";
    }
    if (ext == "png") {
        return "image/png";
    }
    if (ext == "jpg" || ext == "jpeg") {
        return "image/jpeg";
    }
    if (ext == "gif") {
        return "image/gif";
    }
    if (ext == "bmp") {
        return "image/bmp";
    }
    if (ext == "svg") {
        return "image/svg+xml";
    }
    if (ext == "ico") {
        return "image/x-icon";
    }
    if (ext == "json") {
        return "application/json; charset=utf-8";
    }

    return "application/octet-stream";
}
