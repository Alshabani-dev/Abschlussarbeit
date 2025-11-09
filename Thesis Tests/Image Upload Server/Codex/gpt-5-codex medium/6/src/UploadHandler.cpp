#include "UploadHandler.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <ctime>

UploadHandler::UploadHandler()
    : dataRoot_(std::filesystem::current_path() / "Data") {}

void UploadHandler::setDataRoot(const std::filesystem::path& root) {
    dataRoot_ = root;
}

UploadResult UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) const {
    UploadResult result{400, "Invalid upload request", ""};

    const std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        result.message = "400 Bad Request: Missing multipart boundary";
        return result;
    }

    const std::string boundaryMarker = "--" + boundary;
    std::string payload(body.begin(), body.end());

    std::size_t boundaryPos = payload.find(boundaryMarker);
    if (boundaryPos == std::string::npos) {
        result.message = "400 Bad Request: Boundary not found";
        return result;
    }
    boundaryPos += boundaryMarker.length();

    if (boundaryPos + 2 <= payload.size() && payload.compare(boundaryPos, 2, "\r\n") == 0) {
        boundaryPos += 2;
    } else if (boundaryPos + 1 <= payload.size() && payload[boundaryPos] == '\n') {
        ++boundaryPos;
    }

    std::size_t headersEnd = payload.find("\r\n\r\n", boundaryPos);
    std::size_t delimiterLen = 4;
    if (headersEnd == std::string::npos || headersEnd < boundaryPos) {
        headersEnd = payload.find("\n\n", boundaryPos);
        delimiterLen = 2;
    }
    if (headersEnd == std::string::npos || headersEnd < boundaryPos) {
        result.message = "400 Bad Request: Missing multipart headers";
        return result;
    }

    const std::string headersBlock = payload.substr(boundaryPos, headersEnd - boundaryPos);
    std::istringstream headerStream(headersBlock);

    std::string contentDisposition;
    std::string line;
    while (std::getline(headerStream, line)) {
        std::string trimmed = trim(line);
        std::string lower = trimmed;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (lower.find("content-disposition") == 0) {
            contentDisposition = trimmed;
        }
    }

    if (contentDisposition.empty()) {
        result.message = "400 Bad Request: Missing Content-Disposition";
        return result;
    }

    const std::string originalFilename = extractFilename(contentDisposition);
    if (originalFilename.empty()) {
        result.message = "400 Bad Request: Missing filename";
        return result;
    }

    if (!isValidImageExtension(originalFilename)) {
        result.message = "415 Unsupported Media Type: Invalid file extension";
        result.statusCode = 415;
        return result;
    }

    const std::size_t dataStart = headersEnd + delimiterLen;
    if (dataStart > payload.size()) {
        result.message = "400 Bad Request: Malformed multipart body";
        return result;
    }

    std::size_t nextBoundary = payload.find("\r\n" + boundaryMarker, dataStart);
    if (nextBoundary == std::string::npos) {
        nextBoundary = payload.find("\n" + boundaryMarker, dataStart);
    }
    if (nextBoundary == std::string::npos) {
        nextBoundary = payload.find(boundaryMarker + "--", dataStart);
    }

    std::size_t dataEnd = nextBoundary == std::string::npos ? payload.size() : nextBoundary;
    if (dataEnd >= 2 && payload.compare(dataEnd - 2, 2, "\r\n") == 0) {
        dataEnd -= 2;
    } else if (dataEnd >= 1 && payload[dataEnd - 1] == '\n') {
        --dataEnd;
    }

    if (dataEnd < dataStart) {
        result.message = "400 Bad Request: Invalid multipart boundaries";
        return result;
    }

    std::vector<char> fileData(body.begin() + static_cast<std::ptrdiff_t>(dataStart),
                               body.begin() + static_cast<std::ptrdiff_t>(dataEnd));

    if (fileData.empty()) {
        result.message = "400 Bad Request: Empty upload";
        return result;
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuffer{};
#if defined(_WIN32)
    gmtime_s(&tmBuffer, &timestamp);
#else
    gmtime_r(&timestamp, &tmBuffer);
#endif

    std::ostringstream filenameBuilder;
    filenameBuilder << std::put_time(&tmBuffer, "%Y%m%d%H%M%S");
    const std::string safeFilename = sanitizeFilename(originalFilename);
    filenameBuilder << '_' << safeFilename;

    std::filesystem::create_directories(dataRoot_);
    const std::filesystem::path outputPath = dataRoot_ / filenameBuilder.str();
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) {
        throw std::runtime_error("Failed to open output file for upload: " + outputPath.string());
    }
    outFile.write(fileData.data(), static_cast<std::streamsize>(fileData.size()));
    outFile.close();

    result.statusCode = 200;
    result.message = "Upload successful";
    result.storedFilename = outputPath.filename().string();
    return result;
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    const auto dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp";
}

std::string UploadHandler::trim(const std::string& value) {
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }
    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }
    return value.substr(first, last - first);
}

std::string UploadHandler::extractBoundary(const std::string& contentType) {
    const auto boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return {};
    }
    std::string boundary = contentType.substr(boundaryPos + 9);
    if (!boundary.empty() && boundary.front() == '"') {
        boundary.erase(boundary.begin());
    }
    if (!boundary.empty() && boundary.back() == '"') {
        boundary.pop_back();
    }
    return trim(boundary);
}

std::string UploadHandler::extractFilename(const std::string& dispositionLine) {
    const auto filenamePos = dispositionLine.find("filename=");
    if (filenamePos == std::string::npos) {
        return {};
    }
    std::string filename = dispositionLine.substr(filenamePos + 9);
    if (!filename.empty() && filename.front() == '"') {
        filename.erase(filename.begin());
    }
    const auto quotePos = filename.find('"');
    if (quotePos != std::string::npos) {
        filename = filename.substr(0, quotePos);
    }
    const auto lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }
    return trim(filename);
}

std::string UploadHandler::sanitizeFilename(const std::string& filename) {
    std::string sanitized;
    sanitized.reserve(filename.size());
    for (char ch : filename) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '.' || ch == '-' || ch == '_') {
            sanitized.push_back(ch);
        } else {
            sanitized.push_back('_');
        }
    }
    while (!sanitized.empty() && sanitized.front() == '.') {
        sanitized.erase(sanitized.begin());
    }
    if (sanitized.empty()) {
        return "upload.bin";
    }
    return sanitized;
}
