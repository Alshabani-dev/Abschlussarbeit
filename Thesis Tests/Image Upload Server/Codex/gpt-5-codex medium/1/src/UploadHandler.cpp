#include "UploadHandler.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "HttpResponse.h"

namespace {
constexpr std::string_view kBoundaryPrefix = "--";

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    return str.substr(start, end - start);
}
} // namespace

UploadHandler::UploadHandler(std::string dataDir)
    : dataDir_(std::move(dataDir)) {
    std::filesystem::create_directories(dataDir_);
}

HttpResponse UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    HttpResponse response;
    const std::string boundaryToken = extractBoundary(contentType);
    if (boundaryToken.empty()) {
        response.setStatus(400, "Bad Request");
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody("400 Bad Request: Missing multipart boundary");
        return response;
    }

    const std::string boundary = std::string(kBoundaryPrefix) + boundaryToken;
    const std::string payload(body.begin(), body.end());

    const size_t firstBoundary = payload.find(boundary);
    if (firstBoundary == std::string::npos) {
        response.setStatus(400, "Bad Request");
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody("400 Bad Request: Boundary not found");
        return response;
    }

    const size_t partStart = payload.find("\r\n", firstBoundary);
    size_t headersStart = (partStart == std::string::npos) ? std::string::npos : partStart + 2;
    if (headersStart == std::string::npos) {
        response.setStatus(400, "Bad Request");
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody("400 Bad Request: Invalid multipart format");
        return response;
    }

    size_t headersEnd = payload.find("\r\n\r\n", headersStart);
    size_t delimiterLength = 4;
    if (headersEnd == std::string::npos) {
        headersEnd = payload.find("\n\n", headersStart);
        delimiterLength = 2;
    }
    if (headersEnd == std::string::npos) {
        response.setStatus(400, "Bad Request");
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody("400 Bad Request: Missing multipart header delimiter");
        return response;
    }

    const std::string partHeaders = payload.substr(headersStart, headersEnd - headersStart);
    const std::string filename = extractFilename(partHeaders);
    if (filename.empty()) {
        response.setStatus(400, "Bad Request");
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody("400 Bad Request: Filename missing");
        return response;
    }

    if (!isValidImageExtension(filename)) {
        response.setStatus(415, "Unsupported Media Type");
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody("415 Unsupported Media Type");
        return response;
    }

    const size_t fileDataStart = headersEnd + delimiterLength;
    size_t fileDataEnd = payload.find(boundary, fileDataStart);
    if (fileDataEnd == std::string::npos) {
        fileDataEnd = payload.size();
    } else {
        // remove trailing CRLF before boundary
        if (fileDataEnd >= 2 && payload.substr(fileDataEnd - 2, 2) == "\r\n") {
            fileDataEnd -= 2;
        } else if (fileDataEnd >= 1 && payload[fileDataEnd - 1] == '\n') {
            fileDataEnd -= 1;
        }
    }

    if (fileDataEnd <= fileDataStart) {
        response.setStatus(400, "Bad Request");
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody("400 Bad Request: Empty upload");
        return response;
    }

    std::vector<char> fileData(body.begin() + static_cast<std::ptrdiff_t>(fileDataStart),
                               body.begin() + static_cast<std::ptrdiff_t>(fileDataEnd));

    const std::string storedName = persistFile(fileData, filename);

    response.setStatus(200, "OK");
    response.setHeader("Content-Type", "text/plain; charset=utf-8");
    response.setBody("Upload successful: " + storedName);
    return response;
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    const std::string lowered = toLower(filename);
    const std::vector<std::string> allowed = {"jpg", "jpeg", "png", "gif", "bmp"};
    const auto dotPos = lowered.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    const std::string ext = lowered.substr(dotPos + 1);
    return std::find(allowed.begin(), allowed.end(), ext) != allowed.end();
}

std::string UploadHandler::extractBoundary(const std::string& contentType) {
    const std::string target = "boundary=";
    const auto pos = contentType.find(target);
    if (pos == std::string::npos) {
        return {};
    }
    std::string boundary = contentType.substr(pos + target.size());
    if (!boundary.empty() && boundary.front() == '"') {
        const auto quoteEnd = boundary.find('"', 1);
        if (quoteEnd != std::string::npos) {
            boundary = boundary.substr(1, quoteEnd - 1);
        }
    }
    return trim(boundary);
}

std::string UploadHandler::extractFilename(const std::string& partHeaders) {
    const std::string key = "filename=";
    const auto pos = partHeaders.find(key);
    if (pos == std::string::npos) {
        return {};
    }
    size_t start = pos + key.size();
    if (start < partHeaders.size() && partHeaders[start] == '"') {
        ++start;
        const auto end = partHeaders.find('"', start);
        if (end != std::string::npos) {
            return partHeaders.substr(start, end - start);
        }
    }
    const auto end = partHeaders.find(';', start);
    return trim(partHeaders.substr(start, end == std::string::npos ? std::string::npos : end - start));
}

std::string UploadHandler::persistFile(const std::vector<char>& data, const std::string& originalName) const {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    std::filesystem::path original(originalName);
    const std::string baseName = original.filename().string();

    const std::string storedName = std::to_string(millis) + "_" + baseName;
    const std::filesystem::path outputPath = std::filesystem::path(dataDir_) / storedName;

    std::ofstream output(outputPath, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Failed to open output file for upload");
    }
    output.write(data.data(), static_cast<std::streamsize>(data.size()));
    output.close();

    return outputPath.string();
}
