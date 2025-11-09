#include "FileHandler.h"

#include <filesystem>
#include <fstream>
#include <system_error>
#include <unordered_map>

namespace fs = std::filesystem;

FileHandler::FileHandler(std::string publicRoot)
    : publicRoot_(std::filesystem::weakly_canonical(std::filesystem::absolute(std::move(publicRoot)))) {}

bool FileHandler::readFile(const std::string& path, std::vector<char>& out) {
    fs::path target = publicRoot_ / fs::path(path);

    std::error_code ec;
    fs::path normalized = fs::weakly_canonical(target, ec);
    if (ec) {
        normalized = fs::absolute(target);
    }

    const auto normalStr = normalized.generic_string();
    const auto rootStr = publicRoot_.generic_string();
    if (normalStr.compare(0, rootStr.size(), rootStr) != 0) {
        return false;
    }

    if (!fs::exists(normalized) || !fs::is_regular_file(normalized)) {
        return false;
    }

    std::ifstream file(normalized, std::ios::binary);
    if (!file) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

std::string FileHandler::detectMimeType(const std::string& path) const {
    static const std::unordered_map<std::string, std::string> mimeTypes{
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".ico", "image/x-icon"}};

    const fs::path p(path);
    const std::string ext = p.extension().string();
    auto it = mimeTypes.find(ext);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    return "application/octet-stream";
}
