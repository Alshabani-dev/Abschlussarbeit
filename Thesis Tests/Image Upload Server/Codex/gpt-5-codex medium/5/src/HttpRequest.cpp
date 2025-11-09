#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {
std::string trim(const std::string& value) {
    auto begin = value.begin();
    while (begin != value.end() && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    auto end = value.end();
    do {
        if (end == begin) {
            break;
        }
        --end;
    } while (std::isspace(static_cast<unsigned char>(*end)));

    if (begin == value.end()) {
        return {};
    }

    return std::string(begin, end + 1);
}
} // namespace

bool HttpRequest::parse(const std::vector<char>& rawData) {
    auto headerInfo = findHeaderTerminator(rawData);
    if (!headerInfo) {
        return false;
    }

    const size_t headerLength = headerInfo->first;
    const size_t headerEnd = headerInfo->first + headerInfo->second;

    std::string headerSection(rawData.begin(), rawData.begin() + headerLength);

    size_t firstLineEnd = headerSection.find("\r\n");
    size_t delimiterLength = 2;
    if (firstLineEnd == std::string::npos) {
        firstLineEnd = headerSection.find('\n');
        delimiterLength = 1;
    }

    if (firstLineEnd == std::string::npos) {
        return false;
    }

    std::string requestLine = headerSection.substr(0, firstLineEnd);

    size_t methodEnd = requestLine.find(' ');
    if (methodEnd == std::string::npos) {
        return false;
    }
    method_ = requestLine.substr(0, methodEnd);

    size_t pathEnd = requestLine.find(' ', methodEnd + 1);
    if (pathEnd == std::string::npos) {
        return false;
    }
    path_ = requestLine.substr(methodEnd + 1, pathEnd - methodEnd - 1);
    version_ = requestLine.substr(pathEnd + 1);

    size_t offset = firstLineEnd + delimiterLength;
    while (offset < headerSection.size()) {
        size_t nextLineEnd = headerSection.find("\r\n", offset);
        size_t lineDelimLength = 2;
        if (nextLineEnd == std::string::npos) {
            nextLineEnd = headerSection.find('\n', offset);
            lineDelimLength = 1;
        }
        std::string line;
        if (nextLineEnd == std::string::npos) {
            line = headerSection.substr(offset);
            offset = headerSection.size();
        } else {
            line = headerSection.substr(offset, nextLineEnd - offset);
            offset = nextLineEnd + lineDelimLength;
        }

        if (line.empty()) {
            continue;
        }

        size_t separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        std::string key = toLower(trim(line.substr(0, separator)));
        std::string value = trim(line.substr(separator + 1));
        if (!key.empty()) {
            headers_[key] = value;
        }
    }

    body_.assign(rawData.begin() + headerEnd, rawData.end());
    return true;
}

std::optional<std::string> HttpRequest::header(const std::string& key) const {
    auto it = headers_.find(toLower(key));
    if (it == headers_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::pair<size_t, size_t>> HttpRequest::findHeaderTerminator(const std::vector<char>& buffer) {
    const std::string delimiterCRLF = "\r\n\r\n";
    const std::string delimiterLF = "\n\n";

    auto it = std::search(buffer.begin(), buffer.end(), delimiterCRLF.begin(), delimiterCRLF.end());
    if (it != buffer.end()) {
        size_t headerLength = static_cast<size_t>(std::distance(buffer.begin(), it));
        return std::make_pair(headerLength, delimiterCRLF.size());
    }

    it = std::search(buffer.begin(), buffer.end(), delimiterLF.begin(), delimiterLF.end());
    if (it != buffer.end()) {
        size_t headerLength = static_cast<size_t>(std::distance(buffer.begin(), it));
        return std::make_pair(headerLength, delimiterLF.size());
    }

    return std::nullopt;
}

std::optional<size_t> HttpRequest::extractContentLength(const std::string& headers) {
    size_t offset = 0;
    while (offset < headers.size()) {
        size_t lineEnd = headers.find("\r\n", offset);
        size_t lineDelim = 2;
        if (lineEnd == std::string::npos) {
            lineEnd = headers.find('\n', offset);
            lineDelim = 1;
        }

        std::string line;
        if (lineEnd == std::string::npos) {
            line = headers.substr(offset);
            offset = headers.size();
        } else {
            line = headers.substr(offset, lineEnd - offset);
            offset = lineEnd + lineDelim;
        }

        if (line.empty()) {
            continue;
        }

        size_t separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        std::string key = toLower(trim(line.substr(0, separator)));
        if (key != "content-length") {
            continue;
        }

        std::string value = trim(line.substr(separator + 1));
        try {
            size_t idx = 0;
            unsigned long parsed = std::stoul(value, &idx);
            if (idx != value.size()) {
                return std::nullopt;
            }
            return static_cast<size_t>(parsed);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

std::string HttpRequest::extractMethod(const std::string& headers) {
    size_t end = headers.find("\r\n");
    if (end == std::string::npos) {
        end = headers.find('\n');
    }
    std::string line = headers.substr(0, end);
    size_t spacePos = line.find(' ');
    if (spacePos == std::string::npos) {
        return {};
    }
    return line.substr(0, spacePos);
}

std::string HttpRequest::toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
