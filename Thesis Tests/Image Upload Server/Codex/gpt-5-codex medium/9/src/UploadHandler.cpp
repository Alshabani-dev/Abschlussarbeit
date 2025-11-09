#include "UploadHandler.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {

std::string trim(const std::string& input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

std::string normalizeFilename(const std::string& original) {
    std::string sanitized;
    sanitized.reserve(original.size());
    for (char ch : original) {
        if (ch == '/' || ch == '\\') {
            sanitized.clear();
            continue;
        }
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '.' || ch == '_' || ch == '-') {
            sanitized.push_back(ch);
        } else {
            sanitized.push_back('_');
        }
    }
    if (sanitized.empty()) {
        sanitized = "upload";
    }
    return sanitized;
}

std::string generateTimestamp() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const auto time = clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

std::string randomSuffix() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);
    std::string suffix(4, '0');
    for (char& ch : suffix) {
        ch = "0123456789abcdef"[dist(gen)];
    }
    return suffix;
}

} // namespace

UploadHandler::UploadHandler(std::string outputDir) : outputDir_(std::move(outputDir)) {}

UploadResult UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    const std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return {400, "Missing multipart boundary"};
    }

    std::string filename;
    std::vector<char> fileData;
    if (!parseMultipart(body, boundary, filename, fileData)) {
        return {400, "Malformed multipart payload"};
    }

    if (!isValidImageExtension(filename)) {
        return {415, "Unsupported file type"};
    }

    const std::string storedPath = storeFile(filename, fileData);
    if (storedPath.empty()) {
        return {500, "Failed to store uploaded file"};
    }

    return {200, "Upload successful"};
}

bool UploadHandler::parseMultipart(const std::vector<char>& body,
                                   const std::string& boundary,
                                   std::string& filename,
                                   std::vector<char>& fileData) const {
    if (body.empty()) {
        return false;
    }

    const std::string data(body.begin(), body.end());
    const std::string boundaryMarker = "--" + boundary;

    size_t partStart = data.find(boundaryMarker);
    if (partStart == std::string::npos) {
        return false;
    }

    partStart += boundaryMarker.size();
    // Skip potential leading CRLF
    while (partStart < data.size() &&
           (data[partStart] == '\r' || data[partStart] == '\n')) {
        ++partStart;
    }

    const std::string headerDelimiter1 = "\r\n\r\n";
    const std::string headerDelimiter2 = "\n\n";

    size_t headerEnd = data.find(headerDelimiter1, partStart);
    size_t delimiterLength = headerDelimiter1.size();
    if (headerEnd == std::string::npos) {
        headerEnd = data.find(headerDelimiter2, partStart);
        delimiterLength = headerDelimiter2.size();
        if (headerEnd == std::string::npos) {
            return false;
        }
    }

    const std::string headerBlock = data.substr(partStart, headerEnd - partStart);
    std::istringstream headerStream(headerBlock);
    std::string dispositionLine;
    std::string headerLine;
    std::string partName;

    while (std::getline(headerStream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        if (headerLine.empty()) {
            continue;
        }

        const auto colonPos = headerLine.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        std::string key = trim(headerLine.substr(0, colonPos));
        std::string value = trim(headerLine.substr(colonPos + 1));

        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (lowerKey == "content-disposition") {
            dispositionLine = value;
        }
    }

    if (dispositionLine.empty()) {
        return false;
    }

    std::string lowerDisposition = dispositionLine;
    std::transform(lowerDisposition.begin(), lowerDisposition.end(), lowerDisposition.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lowerDisposition.find("form-data") == std::string::npos) {
        return false;
    }

    std::stringstream dispositionStream(dispositionLine);
    std::string segment;
    while (std::getline(dispositionStream, segment, ';')) {
        segment = trim(segment);
        if (segment.empty()) {
            continue;
        }
        std::string lowerSegment = segment;
        std::transform(lowerSegment.begin(), lowerSegment.end(), lowerSegment.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lowerSegment.rfind("name=", 0) == 0) {
            std::string value = segment.substr(5);
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            partName = value;
        } else if (lowerSegment.rfind("filename=", 0) == 0) {
            std::string value = segment.substr(9);
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            filename = normalizeFilename(value);
        }
    }

    if (partName != "file" || filename.empty()) {
        return false;
    }

    const size_t dataStart = headerEnd + delimiterLength;
    const std::string closingBoundary = "\r\n--" + boundary;
    const std::string closingBoundaryLf = "\n--" + boundary;

    size_t dataEnd = data.find(closingBoundary, dataStart);
    if (dataEnd == std::string::npos) {
        dataEnd = data.find(closingBoundaryLf, dataStart);
    }
    if (dataEnd == std::string::npos) {
        return false;
    }

    if (dataEnd < dataStart) {
        return false;
    }

    fileData.assign(body.begin() + static_cast<std::ptrdiff_t>(dataStart),
                    body.begin() + static_cast<std::ptrdiff_t>(dataEnd));
    return true;
}

bool UploadHandler::isValidImageExtension(const std::string& filename) const {
    const auto dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp";
}

std::string UploadHandler::storeFile(const std::string& filename,
                                     const std::vector<char>& data) const {
    if (data.empty()) {
        return {};
    }

    std::error_code ec;
    std::filesystem::create_directories(outputDir_, ec);
    if (ec) {
        return {};
    }

    const std::string finalName =
        generateTimestamp() + "_" + randomSuffix() + "_" + normalizeFilename(filename);
    std::filesystem::path outputPath = std::filesystem::path(outputDir_) / finalName;

    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        return {};
    }
    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
    if (!file) {
        return {};
    }

    return outputPath.generic_string();
}

std::string UploadHandler::extractBoundary(const std::string& contentType) {
    std::string lower = contentType;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const std::string marker = "boundary=";
    const size_t pos = lower.find(marker);
    if (pos == std::string::npos) {
        return {};
    }

    std::string boundary = contentType.substr(pos + marker.size());
    boundary = trim(boundary);
    if (!boundary.empty() && boundary.front() == '"') {
        if (boundary.back() == '"') {
            boundary = boundary.substr(1, boundary.size() - 2);
        } else {
            boundary.erase(boundary.begin());
        }
    }

    // Remove any trailing parameters.
    const auto semicolon = boundary.find(';');
    if (semicolon != std::string::npos) {
        boundary = boundary.substr(0, semicolon);
    }

    return boundary;
}
