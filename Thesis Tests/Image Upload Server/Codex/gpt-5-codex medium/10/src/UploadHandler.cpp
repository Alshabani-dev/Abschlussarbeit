#include "UploadHandler.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>

namespace fs = std::filesystem;

namespace {

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string current;
    std::istringstream stream(text);
    while (std::getline(stream, current)) {
        if (!current.empty() && current.back() == '\r') {
            current.pop_back();
        }
        lines.push_back(current);
    }
    return lines;
}

std::string lowercase(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

size_t findSequence(const std::vector<char>& data, size_t start, const std::string& sequence) {
    if (sequence.empty() || start > data.size()) {
        return std::string::npos;
    }
    auto beginIt = data.begin() + static_cast<std::ptrdiff_t>(start);
    auto it = std::search(beginIt, data.end(), sequence.begin(), sequence.end());
    if (it == data.end()) {
        return std::string::npos;
    }
    return static_cast<size_t>(std::distance(data.begin(), it));
}

} // namespace

UploadHandler::Result UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) const {
    Result result;
    result.statusCode = 400;
    result.reason = "Bad Request";

    const std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        result.message = "Missing multipart boundary";
        return result;
    }

    if (body.empty()) {
        result.message = "Empty payload";
        return result;
    }

    const std::string boundaryToken = "--" + boundary;
    size_t boundaryPos = findSequence(body, 0, boundaryToken);
    if (boundaryPos == std::string::npos) {
        result.message = "Multipart boundary not found";
        return result;
    }

    size_t sectionStart = boundaryPos + boundaryToken.size();
    if (sectionStart > body.size()) {
        result.message = "Multipart section is empty";
        return result;
    }

    const auto hasChar = [&](size_t index, char expected) -> bool {
        return index < body.size() && body[index] == expected;
    };

    if (hasChar(sectionStart, '-') && hasChar(sectionStart + 1, '-')) {
        result.message = "Multipart payload missing file part";
        return result;
    }

    const auto skipLineBreak = [&](size_t pos) -> size_t {
        if (pos + 1 < body.size() && body[pos] == '\r' && body[pos + 1] == '\n') {
            return pos + 2;
        }
        if (pos < body.size() && body[pos] == '\n') {
            return pos + 1;
        }
        return pos;
    };

    sectionStart = skipLineBreak(sectionStart);
    if (sectionStart >= body.size()) {
        result.message = "Multipart section missing headers";
        return result;
    }

    const std::string headerDelimiterCRLF = "\r\n\r\n";
    const std::string headerDelimiterLF = "\n\n";
    size_t headerEnd = findSequence(body, sectionStart, headerDelimiterCRLF);
    size_t delimiterLen = headerDelimiterCRLF.size();
    if (headerEnd == std::string::npos) {
        headerEnd = findSequence(body, sectionStart, headerDelimiterLF);
        delimiterLen = headerDelimiterLF.size();
    }
    if (headerEnd == std::string::npos || headerEnd <= sectionStart) {
        result.message = "Multipart headers not terminated";
        return result;
    }

    const std::string headerSection(body.begin() + static_cast<std::ptrdiff_t>(sectionStart),
                                    body.begin() + static_cast<std::ptrdiff_t>(headerEnd));
    const auto headerLines = splitLines(headerSection);

    std::string filename;
    for (const auto& line : headerLines) {
        const auto lowerLine = lowercase(line);
        const bool dispositionLine = lowerLine.find("content-disposition") != std::string::npos;
        if (!dispositionLine) {
            continue;
        }

        const auto filenamePos = lowerLine.find("filename=");
        if (filenamePos == std::string::npos) {
            continue;
        }

        const auto quoteStart = line.find('"', filenamePos);
        const auto quoteEnd = line.find('"', quoteStart + 1);
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos && quoteEnd > quoteStart) {
            filename = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
    }

    const std::string safeName = sanitizeFilename(filename);
    if (safeName.empty()) {
        result.message = "Filename missing or invalid";
        return result;
    }

    if (!isValidImageExtension(safeName)) {
        result.statusCode = 415;
        result.reason = "Unsupported Media Type";
        result.message = "Unsupported file type";
        return result;
    }

    const size_t dataStart = headerEnd + delimiterLen;
    if (dataStart >= body.size()) {
        result.message = "Multipart payload missing data";
        return result;
    }

    const std::string closingMarkerCRLF = "\r\n" + boundaryToken;
    const std::string closingMarkerLF = "\n" + boundaryToken;
    size_t closingBoundaryPos = findSequence(body, dataStart, closingMarkerCRLF);
    if (closingBoundaryPos == std::string::npos) {
        closingBoundaryPos = findSequence(body, dataStart, closingMarkerLF);
    }
    if (closingBoundaryPos == std::string::npos) {
        result.message = "Closing multipart boundary not found";
        return result;
    }

    size_t dataEnd = closingBoundaryPos;
    while (dataEnd > dataStart && (body[dataEnd - 1] == '\r' || body[dataEnd - 1] == '\n')) {
        --dataEnd;
    }
    if (dataEnd <= dataStart) {
        result.message = "Uploaded file is empty";
        return result;
    }

    const fs::path dataDir("Data");
    std::error_code ec;
    fs::create_directories(dataDir, ec);
    if (ec) {
        result.statusCode = 500;
        result.reason = "Internal Server Error";
        result.message = "Unable to prepare upload directory";
        return result;
    }

    const std::string storedName = timestampedName(safeName);
    const fs::path fullPath = dataDir / storedName;

    std::ofstream output(fullPath, std::ios::binary);
    if (!output.is_open()) {
        result.statusCode = 500;
        result.reason = "Internal Server Error";
        result.message = "Unable to store uploaded file";
        return result;
    }

    const size_t beginOffset = dataStart;
    const size_t endOffset = dataEnd;
    const auto writeSize = static_cast<std::streamsize>(endOffset - beginOffset);
    output.write(body.data() + static_cast<std::ptrdiff_t>(beginOffset), writeSize);
    if (!output.good()) {
        result.statusCode = 500;
        result.reason = "Internal Server Error";
        result.message = "Failed while writing file";
        return result;
    }

    result.success = true;
    result.statusCode = 200;
    result.reason = "OK";
    result.storedFilename = storedName;
    result.message = "Upload successful: " + storedName;
    return result;
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    const auto dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    std::string ext = filename.substr(dotPos + 1);
    ext = lowercase(ext);
    return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp";
}

std::string UploadHandler::extractBoundary(const std::string& contentType) {
    const std::string key = "boundary=";
    const auto boundaryPos = lowercase(contentType).find(key);
    if (boundaryPos == std::string::npos) {
        return {};
    }

    std::string boundary = contentType.substr(boundaryPos + key.size());
    if (!boundary.empty() && boundary.front() == '"') {
        boundary.erase(boundary.begin());
    }
    if (!boundary.empty() && boundary.back() == '"') {
        boundary.pop_back();
    }
    return boundary;
}

std::string UploadHandler::sanitizeFilename(const std::string& filename) {
    if (filename.empty()) {
        return {};
    }

    fs::path path(filename);
    std::string clean = path.filename().string();

    clean.erase(std::remove(clean.begin(), clean.end(), '\0'), clean.end());
    clean.erase(std::remove_if(clean.begin(), clean.end(), [](unsigned char ch) {
        return std::iscntrl(ch);
    }), clean.end());

    return clean;
}

std::string UploadHandler::timestampedName(const std::string& originalName) {
    const auto dotPos = originalName.find_last_of('.');
    const std::string extension = (dotPos == std::string::npos) ? "" : originalName.substr(dotPos);

    const auto now = std::chrono::system_clock::now();
    const auto timeT = std::chrono::system_clock::to_time_t(now);
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif

    std::ostringstream name;
    name << std::put_time(&tm, "%Y%m%d_%H%M%S") << '_' << micros;
    if (!extension.empty()) {
        name << extension;
    }
    return name.str();
}
