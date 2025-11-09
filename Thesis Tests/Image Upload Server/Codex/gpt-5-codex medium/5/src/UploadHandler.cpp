#include "UploadHandler.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <cctype>

UploadHandler::UploadHandler(std::string storageDir)
    : storageDir_(std::filesystem::absolute(std::move(storageDir)).lexically_normal()) {
    storageDirString_ = storageDir_.string();
    if (!storageDirString_.empty() && storageDirString_.back() != std::filesystem::path::preferred_separator) {
        storageDirString_.push_back(std::filesystem::path::preferred_separator);
    }
    std::filesystem::create_directories(storageDir_);
}

UploadResult UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    if (body.empty()) {
        return {false, "Empty request body"};
    }

    const auto boundaryToken = extractBoundary(contentType);
    if (boundaryToken.empty()) {
        return {false, "Invalid multipart boundary"};
    }

    const std::string boundaryMarker = "--" + boundaryToken;
    const std::string data(body.begin(), body.end());

    size_t boundaryPos = data.find(boundaryMarker);
    if (boundaryPos == std::string::npos) {
        return {false, "Multipart boundary not found"};
    }

    boundaryPos += boundaryMarker.size();
    if (data.compare(boundaryPos, 2, "\r\n") == 0) {
        boundaryPos += 2;
    } else if (data.compare(boundaryPos, 1, "\n") == 0) {
        boundaryPos += 1;
    }

    size_t headerEnd = data.find("\r\n\r\n", boundaryPos);
    size_t headerDelimiter = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = data.find("\n\n", boundaryPos);
        headerDelimiter = 2;
    }
    if (headerEnd == std::string::npos) {
        return {false, "Multipart headers not terminated"};
    }

    const std::string headerSection = data.substr(boundaryPos, headerEnd - boundaryPos);

    std::string filename;
    {
        const std::string lowerHeader = toLower(headerSection);
        size_t dispositionPos = lowerHeader.find("content-disposition");
        if (dispositionPos == std::string::npos) {
            return {false, "Missing Content-Disposition header"};
        }

        size_t filenamePos = lowerHeader.find("filename=", dispositionPos);
        if (filenamePos == std::string::npos) {
            return {false, "No filename provided"};
        }

        size_t valueStart = headerSection.find('=', filenamePos);
        if (valueStart == std::string::npos) {
            return {false, "Invalid filename field"};
        }
        valueStart += 1;

        while (valueStart < headerSection.size() && std::isspace(static_cast<unsigned char>(headerSection[valueStart]))) {
            ++valueStart;
        }

        if (valueStart >= headerSection.size()) {
            return {false, "Invalid filename field"};
        }

        if (headerSection[valueStart] == '"' || headerSection[valueStart] == '\'') {
            char quote = headerSection[valueStart];
            ++valueStart;
            size_t valueEnd = headerSection.find(quote, valueStart);
            if (valueEnd == std::string::npos) {
                return {false, "Unterminated filename value"};
            }
            filename = headerSection.substr(valueStart, valueEnd - valueStart);
        } else {
            size_t valueEnd = headerSection.find(';', valueStart);
            if (valueEnd == std::string::npos) {
                valueEnd = headerSection.size();
            }
            filename = headerSection.substr(valueStart, valueEnd - valueStart);
        }
    }

    if (filename.empty()) {
        return {false, "Filename is empty"};
    }

    filename = std::filesystem::path(filename).filename().string();
    if (!isValidImageExtension(filename)) {
        return {false, "Unsupported file extension"};
    }

    const size_t dataStart = headerEnd + headerDelimiter;

    size_t nextBoundary = data.find("\r\n" + boundaryMarker, dataStart);
    if (nextBoundary == std::string::npos) {
        nextBoundary = data.find("\n" + boundaryMarker, dataStart);
    }
    if (nextBoundary == std::string::npos) {
        nextBoundary = data.find(boundaryMarker + "--", dataStart);
    }
    if (nextBoundary == std::string::npos) {
        return {false, "Closing multipart boundary not found"};
    }

    size_t dataEnd = nextBoundary;
    while (dataEnd > dataStart && (data[dataEnd - 1] == '\r' || data[dataEnd - 1] == '\n')) {
        --dataEnd;
    }

    if (dataEnd <= dataStart) {
        return {false, "Uploaded file is empty"};
    }

    const size_t fileSize = dataEnd - dataStart;
    const auto now = std::chrono::system_clock::now();
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::string storedName = std::to_string(timestamp) + "_" + filename;
    std::filesystem::path destination = storageDir_ / storedName;

    std::ofstream output(destination, std::ios::binary);
    if (!output) {
        return {false, "Failed to open destination file"};
    }

    output.write(body.data() + static_cast<std::streamoff>(dataStart), static_cast<std::streamsize>(fileSize));
    if (!output) {
        return {false, "Failed to write uploaded file"};
    }

    return {true, "File uploaded successfully as " + storedName};
}

std::string UploadHandler::trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string UploadHandler::toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string UploadHandler::extractBoundary(const std::string& contentType) const {
    const std::string lower = toLower(contentType);
    const std::string token = "boundary=";
    size_t pos = lower.find(token);
    if (pos == std::string::npos) {
        return {};
    }

    size_t valuePos = pos + token.size();
    std::string boundary = trim(contentType.substr(valuePos));

    size_t semicolon = boundary.find(';');
    if (semicolon != std::string::npos) {
        boundary = boundary.substr(0, semicolon);
    }

    boundary = trim(boundary);
    if (boundary.size() >= 2 && boundary.front() == '"' && boundary.back() == '"') {
        boundary = boundary.substr(1, boundary.size() - 2);
    }

    return boundary;
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    const auto ext = toLower(std::filesystem::path(filename).extension().string());
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp";
}
