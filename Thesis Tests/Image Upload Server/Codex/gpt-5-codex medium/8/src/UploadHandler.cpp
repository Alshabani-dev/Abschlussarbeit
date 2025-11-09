#include "UploadHandler.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {
std::string trim(std::string_view view) {
    const auto begin = view.find_first_not_of(" \t\r\n");
    if (begin == std::string_view::npos) {
        return {};
    }
    const auto end = view.find_last_not_of(" \t\r\n");
    return std::string(view.substr(begin, end - begin + 1));
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}
} // namespace

UploadHandler::UploadHandler(std::string dataDir)
    : dataDir_(std::move(dataDir)) {
    fs::create_directories(dataDir_);
}

UploadHandler::Result UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    Result result;
    if (contentType.empty()) {
        result.message = "400 Bad Request: Missing Content-Type";
        return result;
    }

    std::string lowered = toLower(contentType);
    if (lowered.find("multipart/form-data") != 0) {
        result.message = "400 Bad Request: Unsupported Content-Type";
        return result;
    }

    const std::string boundary = parseBoundary(contentType);
    if (boundary.empty()) {
        result.message = "400 Bad Request: Missing boundary";
        return result;
    }

    const std::string boundaryMarker = "--" + boundary;
    const std::string_view raw(reinterpret_cast<const char*>(body.data()), body.size());

    size_t boundaryPos = raw.find(boundaryMarker);
    if (boundaryPos == std::string_view::npos) {
        result.message = "400 Bad Request: Boundary not found";
        return result;
    }

    size_t cursor = boundaryPos + boundaryMarker.size();
    if (cursor + 2 <= raw.size() && raw.substr(cursor, 2) == "\r\n") {
        cursor += 2;
    } else if (cursor + 1 <= raw.size() && raw.substr(cursor, 1) == "\n") {
        cursor += 1;
    }

    if (cursor >= raw.size()) {
        result.message = "400 Bad Request: Malformed multipart body";
        return result;
    }

    size_t headersEnd = raw.find("\r\n\r\n", cursor);
    size_t delimiterLen = 4;
    if (headersEnd == std::string::npos) {
        headersEnd = raw.find("\n\n", cursor);
        if (headersEnd == std::string::npos) {
            result.message = "400 Bad Request: No multipart headers";
            return result;
        }
        delimiterLen = 2;
    }

    if (headersEnd < cursor) {
        result.message = "400 Bad Request: Invalid header section";
        return result;
    }

    const std::string_view headerView = raw.substr(cursor, headersEnd - cursor);
    const std::string partHeaders(headerView.begin(), headerView.end());
    const std::string filename = extractFilename(partHeaders);
    if (filename.empty()) {
        result.message = "400 Bad Request: Filename missing";
        return result;
    }
    if (!isValidImageExtension(filename)) {
        result.message = "400 Bad Request: Invalid file type";
        return result;
    }

    const size_t fileDataStart = headersEnd + delimiterLen;
    if (fileDataStart > raw.size()) {
        result.message = "400 Bad Request: Invalid body length";
        return result;
    }

    const std::string boundaryWithCrlf = "\r\n" + boundaryMarker;
    const std::string boundaryWithLf = "\n" + boundaryMarker;

    size_t fileDataEnd = raw.find(boundaryWithCrlf, fileDataStart);
    if (fileDataEnd == std::string::npos) {
        fileDataEnd = raw.find(boundaryWithLf, fileDataStart);
    }
    if (fileDataEnd == std::string::npos) {
        fileDataEnd = raw.find(boundaryMarker, fileDataStart);
    }
    if (fileDataEnd == std::string::npos) {
        fileDataEnd = raw.size();
    }

    if (fileDataEnd <= fileDataStart) {
        result.message = "400 Bad Request: Invalid body";
        return result;
    }

    const auto startIt = body.begin() + static_cast<std::ptrdiff_t>(fileDataStart);
    const auto endIt = body.begin() + static_cast<std::ptrdiff_t>(fileDataEnd);
    std::vector<char> fileData(startIt, endIt);
    if (fileData.empty()) {
        result.message = "400 Bad Request: Empty file";
        return result;
    }

    const std::string storedPath = writeFile(filename, fileData);
    if (storedPath.empty()) {
        result.message = "500 Internal Server Error: Unable to store file";
        return result;
    }

    result.success = true;
    result.message = "Upload successful: " + storedPath;
    return result;
}

std::string UploadHandler::parseBoundary(const std::string& contentType) {
    const std::string lowered = toLower(contentType);
    const std::string key = "boundary=";

    auto pos = lowered.find(key);
    if (pos == std::string::npos) {
        return {};
    }

    pos += key.size();
    const auto end = lowered.find(';', pos);
    const std::string_view rawBoundary(contentType.data() + pos, (end == std::string::npos ? contentType.size() : end) - pos);
    std::string boundary = trim(rawBoundary);
    if (!boundary.empty() && boundary.front() == '"') {
        boundary.erase(boundary.begin());
    }
    if (!boundary.empty() && boundary.back() == '"') {
        boundary.pop_back();
    }
    return boundary;
}

std::string UploadHandler::extractFilename(const std::string& headers) {
    const std::string key = "filename=\"";
    const auto pos = headers.find(key);
    if (pos == std::string::npos) {
        return {};
    }
    const auto end = headers.find('"', pos + key.size());
    if (end == std::string::npos) {
        return {};
    }
    return headers.substr(pos + key.size(), end - (pos + key.size()));
}

bool UploadHandler::isValidImageExtension(const std::string& filename) const {
    const std::vector<std::string> valid{".jpg", ".jpeg", ".png", ".gif", ".bmp"};
    const auto dot = filename.find_last_of('.');
    if (dot == std::string::npos) {
        return false;
    }
    std::string ext = filename.substr(dot);
    for (auto& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    for (const auto& validExt : valid) {
        if (ext == validExt) {
            return true;
        }
    }
    return false;
}

std::string UploadHandler::writeFile(const std::string& filename, const std::vector<char>& data) const {
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();
    const fs::path safeName = fs::path(filename).filename();
    const fs::path target = fs::path(dataDir_) / (std::to_string(timestamp) + "_" + safeName.string());
    std::ofstream out(target, std::ios::binary);
    if (!out) {
        return {};
    }
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!out) {
        return {};
    }
    return target.string();
}
