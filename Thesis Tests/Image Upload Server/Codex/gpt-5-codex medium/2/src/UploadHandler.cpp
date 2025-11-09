#include "UploadHandler.h"

#include "HttpResponse.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace {

std::string trim(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

} // namespace

UploadHandler::UploadHandler()
    : uploadsDir_("Data") {}

HttpResponse UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    if (contentType.empty()) {
        return badRequest("Missing Content-Type");
    }

    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return badRequest("Invalid multipart boundary");
    }

    std::string data(body.begin(), body.end());
    std::string boundaryMarker = "--" + boundary;

    std::size_t partStart = data.find(boundaryMarker);
    if (partStart == std::string::npos) {
        return badRequest("Boundary not found");
    }

    partStart += boundaryMarker.size();
    if (partStart + 1 >= data.size()) {
        return badRequest("Malformed multipart payload");
    }

    if (data.compare(partStart, 2, "\r\n") == 0) {
        partStart += 2;
    } else if (data[partStart] == '\n') {
        partStart += 1;
    }

    std::size_t headerEnd = data.find("\r\n\r\n", partStart);
    std::size_t delimiterLength = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = data.find("\n\n", partStart);
        delimiterLength = 2;
    }
    if (headerEnd == std::string::npos) {
        return badRequest("Missing part headers");
    }

    std::string partHeader = data.substr(partStart, headerEnd - partStart);
    std::size_t dispositionPos = partHeader.find("Content-Disposition");
    if (dispositionPos == std::string::npos) {
        return badRequest("Missing Content-Disposition");
    }

    std::string disposition = partHeader.substr(dispositionPos);
    std::size_t namePos = disposition.find("name=");
    std::size_t filenamePos = disposition.find("filename=");
    if (namePos == std::string::npos || filenamePos == std::string::npos) {
        return badRequest("Invalid Content-Disposition");
    }

    std::size_t nameStart = disposition.find('"', namePos);
    std::size_t nameEnd = disposition.find('"', nameStart + 1);
    if (nameStart == std::string::npos || nameEnd == std::string::npos) {
        return badRequest("Malformed name field");
    }

    std::string fieldName = disposition.substr(nameStart + 1, nameEnd - nameStart - 1);
    if (fieldName != "file") {
        return badRequest("Unexpected form field");
    }

    std::size_t filenameStart = disposition.find('"', filenamePos);
    std::size_t filenameEnd = disposition.find('"', filenameStart + 1);
    if (filenameStart == std::string::npos || filenameEnd == std::string::npos) {
        return badRequest("Missing filename");
    }

    std::string filename = disposition.substr(filenameStart + 1, filenameEnd - filenameStart - 1);
    if (filename.empty()) {
        return badRequest("Empty filename");
    }

    std::filesystem::path safeFilename = std::filesystem::path(filename).filename();
    std::string extension = safeFilename.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (!isValidImageExtension(extension)) {
        return badRequest("Unsupported file type");
    }

    std::size_t dataStart = headerEnd + delimiterLength;
    std::size_t dataEnd = data.find(boundaryMarker, dataStart);
    if (dataEnd == std::string::npos) {
        dataEnd = data.size();
    }

    std::size_t trimmedEnd = dataEnd;
    if (trimmedEnd >= 2 && data[trimmedEnd - 2] == '\r' && data[trimmedEnd - 1] == '\n') {
        trimmedEnd -= 2;
    } else if (trimmedEnd >= 1 && data[trimmedEnd - 1] == '\n') {
        trimmedEnd -= 1;
    }

    std::vector<char> fileBuffer(body.begin() + static_cast<long>(dataStart), body.begin() + static_cast<long>(trimmedEnd));
    if (fileBuffer.empty()) {
        return badRequest("Empty file payload");
    }

    std::filesystem::create_directories(uploadsDir_);
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    std::string storedName = std::to_string(timestamp) + "_" + safeFilename.string();
    std::filesystem::path fullPath = std::filesystem::path(uploadsDir_) / storedName;

    std::ofstream output(fullPath, std::ios::binary);
    if (!output.is_open()) {
        return serverError("Failed to save file");
    }

    output.write(fileBuffer.data(), static_cast<std::streamsize>(fileBuffer.size()));
    if (!output) {
        return serverError("Write failure");
    }

    HttpResponse response(200, "OK");
    response.setHeader("Content-Type", "text/plain; charset=utf-8");
    response.setBody("Upload successful");
    return response;
}

bool UploadHandler::isValidImageExtension(const std::string& extension) const {
    static const std::vector<std::string> allowed = {".jpg", ".jpeg", ".png", ".gif", ".bmp"};
    return std::find(allowed.begin(), allowed.end(), extension) != allowed.end();
}

std::string UploadHandler::extractBoundary(const std::string& contentType) const {
    std::string lowered = contentType;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (lowered.find("multipart/form-data") == std::string::npos) {
        return {};
    }

    std::size_t boundaryPos = lowered.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return {};
    }

    std::size_t valueStart = boundaryPos + 9;
    std::string boundary = trim(contentType.substr(valueStart));
    if (!boundary.empty() && boundary.front() == '"') {
        boundary.erase(boundary.begin());
    }
    if (!boundary.empty() && boundary.back() == '"') {
        boundary.pop_back();
    }

    std::size_t semicolon = boundary.find(';');
    if (semicolon != std::string::npos) {
        boundary = boundary.substr(0, semicolon);
    }

    boundary = trim(boundary);
    return boundary;
}

HttpResponse UploadHandler::badRequest(const std::string& message) const {
    HttpResponse response(400, "Bad Request");
    response.setHeader("Content-Type", "text/plain; charset=utf-8");
    response.setBody(message);
    return response;
}

HttpResponse UploadHandler::serverError(const std::string& message) const {
    HttpResponse response(500, "Internal Server Error");
    response.setHeader("Content-Type", "text/plain; charset=utf-8");
    response.setBody(message);
    return response;
}
