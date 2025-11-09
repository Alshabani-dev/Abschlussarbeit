#include "UploadHandler.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>

namespace {
HttpResponse makeErrorResponse(int statusCode, const std::string& message) {
    HttpResponse response(statusCode, statusCode == 400 ? "Bad Request" : statusCode == 404 ? "Not Found" : statusCode == 415 ? "Unsupported Media Type" : statusCode == 500 ? "Internal Server Error" : "Bad Request");
    response.setBody(message, "text/plain; charset=utf-8");
    return response;
}
} // namespace

UploadHandler::UploadHandler(std::string outputDirectory)
    : outputDirectory_(std::move(outputDirectory)) {}

HttpResponse UploadHandler::handle(const HttpRequest& request) const {
    const auto contentTypeOpt = request.header("Content-Type");
    if (!contentTypeOpt.has_value()) {
        return makeErrorResponse(400, "Missing Content-Type header");
    }

    const auto boundaryOpt = extractBoundary(contentTypeOpt.value());
    if (!boundaryOpt.has_value()) {
        return makeErrorResponse(400, "Boundary not provided");
    }

    const std::string boundaryMarker = "--" + boundaryOpt.value();
    const std::string closingBoundaryMarker = boundaryMarker + "--";

    const auto& body = request.body();
    if (body.empty()) {
        return makeErrorResponse(400, "Empty request body");
    }

    // Safe because std::string can hold null bytes and preserves length.
    const std::string bodyString(body.begin(), body.end());

    auto sectionPosition = bodyString.find(boundaryMarker);
    if (sectionPosition == std::string::npos) {
        return makeErrorResponse(400, "Boundary not found in body");
    }
    sectionPosition += boundaryMarker.size();

    // Skip initial newline(s) after boundary marker.
    if (sectionPosition + 1 < bodyString.size()) {
        if (bodyString.compare(sectionPosition, 2, "\r\n") == 0) {
            sectionPosition += 2;
        } else if (bodyString[sectionPosition] == '\n') {
            sectionPosition += 1;
        }
    }

    std::size_t delimiterLength = 4;
    std::size_t partHeaderEnd = bodyString.find("\r\n\r\n", sectionPosition);
    if (partHeaderEnd == std::string::npos) {
        delimiterLength = 2;
        partHeaderEnd = bodyString.find("\n\n", sectionPosition);
    }
    if (partHeaderEnd == std::string::npos) {
        return makeErrorResponse(400, "Multipart headers not terminated correctly");
    }

    const std::string partHeaders = bodyString.substr(sectionPosition, partHeaderEnd - sectionPosition);

    const auto filenameOpt = extractFilename(partHeaders);
    if (!filenameOpt.has_value() || filenameOpt->empty()) {
        return makeErrorResponse(400, "Filename missing in multipart data");
    }

    const std::string sanitizedFilename = sanitizeFilename(filenameOpt.value());
    if (sanitizedFilename.empty()) {
        return makeErrorResponse(400, "Filename is invalid after sanitization");
    }

    if (!isValidImageExtension(sanitizedFilename)) {
        return makeErrorResponse(415, "Unsupported file extension");
    }

    const std::size_t dataStart = partHeaderEnd + delimiterLength;
    auto nextBoundaryPos = bodyString.find(boundaryMarker, dataStart);
    if (nextBoundaryPos == std::string::npos) {
        nextBoundaryPos = bodyString.find(closingBoundaryMarker, dataStart);
        if (nextBoundaryPos == std::string::npos) {
            return makeErrorResponse(400, "Could not locate multipart boundary ending");
        }
    }

    std::size_t dataEnd = nextBoundaryPos;
    while (dataEnd > dataStart &&
           (body[dataEnd - 1] == '\r' || body[dataEnd - 1] == '\n')) {
        --dataEnd;
    }

    if (dataEnd <= dataStart) {
        return makeErrorResponse(400, "No data found for uploaded file");
    }

    std::vector<char> fileData(body.begin() + static_cast<std::ptrdiff_t>(dataStart),
                               body.begin() + static_cast<std::ptrdiff_t>(dataEnd));

    // Build output filename with timestamp prefix.
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm timeBuffer{};
#if defined(_MSC_VER)
    localtime_s(&timeBuffer, &nowTime);
#else
    timeBuffer = *std::localtime(&nowTime);
#endif

    std::ostringstream filenameBuilder;
    filenameBuilder << std::put_time(&timeBuffer, "%Y%m%d%H%M%S") << "_" << sanitizedFilename;
    const std::string finalFilename = filenameBuilder.str();

    const std::string outputPath = outputDirectory_ + "/" + finalFilename;
    std::ofstream outputStream(outputPath, std::ios::binary);
    if (!outputStream) {
        return makeErrorResponse(500, "Failed to open destination file for writing");
    }
    outputStream.write(fileData.data(), static_cast<std::streamsize>(fileData.size()));
    if (!outputStream) {
        return makeErrorResponse(500, "Failed while writing the uploaded file");
    }

    HttpResponse response(200, "OK");
    response.setBody("Upload successful", "text/plain; charset=utf-8");
    return response;
}

std::optional<std::string> UploadHandler::extractBoundary(const std::string& contentTypeHeader) {
    const auto boundaryPos = contentTypeHeader.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return std::nullopt;
    }

    std::string boundary = contentTypeHeader.substr(boundaryPos + 9);
    // Trim whitespace.
    const auto first = boundary.find_first_not_of(" \"");
    const auto last = boundary.find_last_not_of(" \"");
    if (first == std::string::npos || last == std::string::npos) {
        return std::nullopt;
    }

    boundary = boundary.substr(first, last - first + 1);
    return boundary;
}

std::optional<std::string> UploadHandler::extractFilename(const std::string& partHeaders) {
    std::istringstream stream(partHeaders);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("Content-Disposition") == std::string::npos) {
            continue;
        }

        const auto filenamePos = line.find("filename=");
        if (filenamePos == std::string::npos) {
            continue;
        }

        std::string filename = line.substr(filenamePos + 9);

        // Trim surrounding whitespace and carriage returns.
        const auto first = filename.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            filename.erase(0, first);
        } else {
            filename.clear();
        }
        const auto last = filename.find_last_not_of(" \t\r\n");
        if (last != std::string::npos) {
            filename.erase(last + 1);
        }

        if (!filename.empty() && filename.front() == '"') {
            filename.erase(filename.begin());
        }
        if (!filename.empty() && filename.back() == '"') {
            filename.pop_back();
        }

        return filename;
    }
    return std::nullopt;
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    const auto dotPosition = filename.find_last_of('.');
    if (dotPosition == std::string::npos) {
        return false;
    }

    std::string extension = filename.substr(dotPosition + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "gif" || extension == "bmp";
}

std::string UploadHandler::sanitizeFilename(const std::string& filename) {
    std::string result;
    result.reserve(filename.size());

    for (char c : filename) {
        if (c == '/' || c == '\\') {
            continue;
        }
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' || c == '-') {
            result.push_back(c);
        } else {
            result.push_back('_');
        }
    }

    // Remove leading dots to avoid hidden files.
    while (!result.empty() && result.front() == '.') {
        result.erase(result.begin());
    }

    return result;
}
