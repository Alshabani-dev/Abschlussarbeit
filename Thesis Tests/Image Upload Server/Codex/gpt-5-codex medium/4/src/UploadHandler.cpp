#include "UploadHandler.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string_view>

namespace {
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

void trim(std::string& value) {
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    value.erase(std::find_if(value.rbegin(), value.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                value.end());
}
} // namespace

UploadHandler::Result UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) const {
    if (body.empty()) {
        return {false, "Empty request body"};
    }

    if (contentType.empty()) {
        return {false, "Missing Content-Type header"};
    }

    std::string lowerContentType = toLower(contentType);
    if (lowerContentType.find("multipart/form-data") != 0) {
        return {false, "Unsupported Content-Type"};
    }

    const std::string boundaryKey = "boundary=";
    const auto boundaryPos = lowerContentType.find(boundaryKey);
    if (boundaryPos == std::string::npos) {
        return {false, "Missing multipart boundary"};
    }

    std::string boundary = contentType.substr(boundaryPos + boundaryKey.size());
    trim(boundary);
    if (!boundary.empty() && boundary.front() == '"' && boundary.back() == '"') {
        boundary = boundary.substr(1, boundary.size() - 2);
    }
    const std::string delimiter = "--" + boundary;

    std::string data(body.begin(), body.end());
    size_t searchPos = 0;
    while (true) {
        size_t boundaryPosBody = data.find(delimiter, searchPos);
        if (boundaryPosBody == std::string::npos) {
            break;
        }
        size_t partStart = boundaryPosBody + delimiter.size();
        if (partStart >= data.size()) {
            break;
        }
        if (data.compare(partStart, 2, "--") == 0) {
            break; // reached final boundary
        }
        if (data.compare(partStart, 2, "\r\n") == 0) {
            partStart += 2;
        } else if (data.compare(partStart, 1, "\n") == 0) {
            partStart += 1;
        }

        size_t headerDelimiter = 4;
        size_t headerEnd = data.find("\r\n\r\n", partStart);
        if (headerEnd == std::string::npos) {
            headerDelimiter = 2;
            headerEnd = data.find("\n\n", partStart);
        }
        if (headerEnd == std::string::npos) {
            return {false, "Malformed multipart body"};
        }

        const std::string headerBlock = data.substr(partStart, headerEnd - partStart);
        size_t dataStart = headerEnd + headerDelimiter;
        size_t nextBoundary = data.find(delimiter, dataStart);
        size_t rawDataEnd = (nextBoundary == std::string::npos) ? data.size() : nextBoundary;

        size_t dataEnd = rawDataEnd;
        if (dataEnd >= 2 && data[dataEnd - 2] == '\r' && data[dataEnd - 1] == '\n') {
            dataEnd -= 2;
        } else if (dataEnd >= 1 && data[dataEnd - 1] == '\n') {
            dataEnd -= 1;
        }
        if (dataEnd < dataStart) {
            dataEnd = dataStart;
        }

        std::istringstream headerStream(headerBlock);
        std::string headerLine;
        std::string nameField;
        std::string filename;

        while (std::getline(headerStream, headerLine)) {
            if (!headerLine.empty() && headerLine.back() == '\r') {
                headerLine.pop_back();
            }
            if (headerLine.empty()) {
                continue;
            }
            const auto colon = headerLine.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            std::string key = headerLine.substr(0, colon);
            std::string value = headerLine.substr(colon + 1);
            trim(key);
            trim(value);

            if (toLower(key) == "content-disposition") {
                std::istringstream dispositionStream(value);
                std::string token;
                while (std::getline(dispositionStream, token, ';')) {
                    trim(token);
                    if (token.rfind("name=", 0) == 0) {
                        nameField = token.substr(5);
                        if (!nameField.empty() && nameField.front() == '"' && nameField.back() == '"') {
                            nameField = nameField.substr(1, nameField.size() - 2);
                        }
                    } else if (token.rfind("filename=", 0) == 0) {
                        filename = token.substr(9);
                        if (!filename.empty() && filename.front() == '"' && filename.back() == '"') {
                            filename = filename.substr(1, filename.size() - 2);
                        }
                    }
                }
            }
        }

        if (nameField == "file" && !filename.empty()) {
            if (!isValidImageExtension(filename)) {
                return {false, "Invalid image file extension"};
            }

            std::filesystem::create_directories("Data");
            const std::string sanitized = sanitizeFilename(filename);
            const std::string targetPath = generateTargetPath(sanitized);

            std::ofstream output(targetPath, std::ios::binary);
            if (!output) {
                return {false, "Unable to open destination file"};
            }

            const size_t offsetStart = dataStart;
            const size_t offsetEnd = dataEnd;
            output.write(body.data() + static_cast<std::ptrdiff_t>(offsetStart),
                         static_cast<std::streamsize>(offsetEnd - offsetStart));
            if (!output) {
                return {false, "Failed while writing uploaded file"};
            }

            return {true, "Upload successful: " + targetPath};
        }

        if (nextBoundary == std::string::npos) {
            break;
        }
        searchPos = nextBoundary;
    }

    return {false, "No valid file field named 'file' found"};
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    std::filesystem::path path(filename);
    std::string ext = toLower(path.extension().string());
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp";
}

std::string UploadHandler::sanitizeFilename(std::string filename) {
    std::filesystem::path path(filename);
    std::string leaf = path.filename().string();
    std::string result;
    result.reserve(leaf.size());
    for (char ch : leaf) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '.' || ch == '_' || ch == '-') {
            result.push_back(ch);
        } else {
            result.push_back('_');
        }
    }
    if (result.empty()) {
        result = "upload.bin";
    }
    return result;
}

std::string UploadHandler::generateTargetPath(const std::string& sanitizedName) {
    using std::chrono::system_clock;
    using std::chrono::milliseconds;

    const auto now = system_clock::now();
    const auto epochMs = std::chrono::duration_cast<milliseconds>(now.time_since_epoch()).count();

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1000, 9999);

    std::ostringstream oss;
    oss << "Data/" << epochMs << '_' << dist(rng) << '_' << sanitizedName;
    return oss.str();
}
