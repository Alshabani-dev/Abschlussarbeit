#include "UploadHandler.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace {

void trim(std::string& value) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    auto begin = std::find_if(value.begin(), value.end(), notSpace);
    auto end = std::find_if(value.rbegin(), value.rend(), notSpace).base();
    if (begin >= end) {
        value.clear();
        return;
    }
    value.assign(begin, end);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string extractParameter(const std::string& headerValue, const std::string& key) {
    std::string lowerHeader = toLower(headerValue);
    std::string lowerKey = toLower(key) + "=";

    auto pos = lowerHeader.find(lowerKey);
    if (pos == std::string::npos) {
        return {};
    }
    pos += lowerKey.size();

    if (pos >= headerValue.size()) {
        return {};
    }

    std::string value = headerValue.substr(pos);
    auto semicolon = value.find(';');
    if (semicolon != std::string::npos) {
        value = value.substr(0, semicolon);
    }
    trim(value);

    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }

    return value;
}

std::string escapeFilename(const std::string& filename) {
    std::string sanitized;
    sanitized.reserve(filename.size());
    for (char ch : filename) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '.' || ch == '-' || ch == '_') {
            sanitized.push_back(ch);
        }
    }
    return sanitized;
}

std::string generatePrefix() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::mt19937 rng(static_cast<unsigned int>(ms));
    std::uniform_int_distribution<int> dist(0, 9999);

    std::ostringstream oss;
    oss << ms << "_" << dist(rng);
    return oss.str();
}

size_t skipLineBreak(const std::string& data, size_t position) {
    if (position >= data.size()) {
        return position;
    }
    if (data.compare(position, 2, "\r\n") == 0) {
        return position + 2;
    }
    if (data[position] == '\n') {
        return position + 1;
    }
    return position;
}

}  // namespace

UploadHandler::UploadHandler(const std::string& dataDirectory) : dataDirectory_(dataDirectory) {}

UploadHandler::~UploadHandler() = default;

UploadHandler::Result UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) const {
    if (body.empty()) {
        return {false, "Empty upload payload.", ""};
    }

    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return {false, "Missing multipart boundary.", ""};
    }

    std::string delimiter = "--" + boundary;
    std::string closingDelimiter = delimiter + "--";

    std::string data(body.begin(), body.end());

    size_t boundaryPos = data.find(delimiter);
    if (boundaryPos == std::string::npos) {
        return {false, "Multipart boundary not found in payload.", ""};
    }

    size_t partStart = boundaryPos + delimiter.size();
    partStart = skipLineBreak(data, partStart);

    size_t headersEnd = data.find("\r\n\r\n", partStart);
    size_t delimiterLength = 4;
    if (headersEnd == std::string::npos) {
        headersEnd = data.find("\n\n", partStart);
        delimiterLength = 2;
    }
    if (headersEnd == std::string::npos) {
        return {false, "Malformed multipart headers.", ""};
    }

    std::string headersSection = data.substr(partStart, headersEnd - partStart);
    std::unordered_map<std::string, std::string> headers;
    std::istringstream headerStream(headersSection);
    std::string headerLine;

    while (std::getline(headerStream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        auto colonPos = headerLine.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        std::string key = headerLine.substr(0, colonPos);
        std::string value = headerLine.substr(colonPos + 1);
        trim(key);
        trim(value);
        headers[toLower(key)] = value;
    }

    auto dispositionIt = headers.find("content-disposition");
    if (dispositionIt == headers.end()) {
        return {false, "Missing Content-Disposition header.", ""};
    }

    std::string fieldName = extractParameter(dispositionIt->second, "name");
    if (fieldName != "file") {
        return {false, "Expected form field named 'file'.", ""};
    }

    std::string originalFilename = extractParameter(dispositionIt->second, "filename");
    if (originalFilename.empty()) {
        return {false, "Filename missing in upload request.", ""};
    }

    if (!isValidImageExtension(originalFilename)) {
        return {false, "Unsupported file extension.", ""};
    }

    std::string sanitizedName = sanitizeFilename(originalFilename);
    if (sanitizedName.empty()) {
        sanitizedName = "upload";
    }

    size_t dataStart = headersEnd + delimiterLength;

    size_t dataEnd = data.find("\r\n" + delimiter, dataStart);
    if (dataEnd == std::string::npos) {
        dataEnd = data.find("\n" + delimiter, dataStart);
    }
    if (dataEnd == std::string::npos) {
        dataEnd = data.find(closingDelimiter, dataStart);
    }
    if (dataEnd == std::string::npos) {
        dataEnd = data.size();
    }

    // Trim trailing line breaks that may precede the boundary
    while (dataEnd > dataStart && (data[dataEnd - 1] == '\r' || data[dataEnd - 1] == '\n')) {
        --dataEnd;
    }

    if (dataEnd < dataStart || dataEnd > body.size()) {
        return {false, "Invalid file boundaries in upload payload.", ""};
    }

    std::vector<char> fileData(body.begin() + static_cast<std::ptrdiff_t>(dataStart),
                               body.begin() + static_cast<std::ptrdiff_t>(dataEnd));

    if (fileData.empty()) {
        return {false, "Uploaded file is empty.", ""};
    }

    std::error_code ec;
    std::filesystem::create_directories(dataDirectory_, ec);
    if (ec) {
        return {false, "Unable to create upload directory.", ""};
    }

    std::string uniqueName = generatePrefix() + "_" + sanitizedName;
    std::filesystem::path outputPath = std::filesystem::path(dataDirectory_) / uniqueName;

    std::ofstream output(outputPath, std::ios::binary);
    if (!output) {
        return {false, "Failed to open destination file for writing.", ""};
    }

    output.write(fileData.data(), static_cast<std::streamsize>(fileData.size()));
    if (!output.good()) {
        return {false, "Failed to persist uploaded file.", ""};
    }

    return {true, "Upload successful.", uniqueName};
}

bool UploadHandler::isValidImageExtension(const std::string& filename) const {
    auto dot = filename.find_last_of('.');
    if (dot == std::string::npos || dot == filename.size() - 1) {
        return false;
    }
    std::string extension = toLower(filename.substr(dot + 1));
    static const std::unordered_set<std::string> kAllowed = {"jpg", "jpeg", "png", "gif", "bmp"};
    return kAllowed.find(extension) != kAllowed.end();
}

std::string UploadHandler::extractBoundary(const std::string& contentType) const {
    if (contentType.empty()) {
        return {};
    }

    std::string lower = toLower(contentType);
    auto boundaryPos = lower.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return {};
    }
    boundaryPos += std::string("boundary=").size();

    std::string boundary = contentType.substr(boundaryPos);
    auto semicolon = boundary.find(';');
    if (semicolon != std::string::npos) {
        boundary = boundary.substr(0, semicolon);
    }
    trim(boundary);

    if (boundary.size() >= 2 && boundary.front() == '"' && boundary.back() == '"') {
        boundary = boundary.substr(1, boundary.size() - 2);
    }

    return boundary;
}

std::string UploadHandler::sanitizeFilename(const std::string& filename) const {
    std::string sanitized = escapeFilename(filename);
    while (!sanitized.empty() && sanitized.front() == '.') {
        sanitized.erase(sanitized.begin());
    }
    if (sanitized.empty()) {
        return {};
    }
    return sanitized;
}
