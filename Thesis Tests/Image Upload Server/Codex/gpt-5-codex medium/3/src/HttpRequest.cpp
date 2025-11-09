#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace {

struct HeaderBoundary {
    size_t headerBytes;
    size_t totalLength;
};

std::optional<HeaderBoundary> locateHeaderBoundary(const std::vector<char>& buffer) {
    static const std::string kCRLF = "\r\n\r\n";
    static const std::string kLF = "\n\n";

    auto it = std::search(buffer.begin(), buffer.end(), kCRLF.begin(), kCRLF.end());
    if (it != buffer.end()) {
        size_t headerBytes = static_cast<size_t>(std::distance(buffer.begin(), it));
        return HeaderBoundary{headerBytes, headerBytes + kCRLF.size()};
    }

    it = std::search(buffer.begin(), buffer.end(), kLF.begin(), kLF.end());
    if (it != buffer.end()) {
        size_t headerBytes = static_cast<size_t>(std::distance(buffer.begin(), it));
        return HeaderBoundary{headerBytes, headerBytes + kLF.size()};
    }

    return std::nullopt;
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

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

std::vector<std::string> splitLines(const std::string& input) {
    std::vector<std::string> lines;
    std::string current;
    std::istringstream stream(input);
    while (std::getline(stream, current)) {
        if (!current.empty() && current.back() == '\r') {
            current.pop_back();
        }
        lines.push_back(current);
    }
    return lines;
}

size_t parseContentLength(const std::unordered_map<std::string, std::string>& headers) {
    auto it = headers.find("content-length");
    if (it == headers.end()) {
        return 0;
    }
    const std::string& value = it->second;
    if (value.empty()) {
        return 0;
    }
    size_t length = 0;
    try {
        length = static_cast<size_t>(std::stoul(value));
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid Content-Length value");
    }
    return length;
}

}  // namespace

std::string HttpRequest::headerValue(const std::string& key) const {
    auto it = headers_.find(toLower(key));
    if (it == headers_.end()) {
        return {};
    }
    return it->second;
}

bool HttpRequest::tryParse(const std::vector<char>& buffer, size_t& requestLength, HttpRequest& request) {
    requestLength = 0;
    auto boundary = locateHeaderBoundary(buffer);
    if (!boundary) {
        return false;
    }

    request.reset();

    std::string headerSection(buffer.begin(), buffer.begin() + boundary->headerBytes);
    std::vector<std::string> lines = splitLines(headerSection);

    if (lines.empty()) {
        throw std::runtime_error("Empty request line");
    }

    std::istringstream requestLine(lines.front());
    if (!(requestLine >> request.method_ >> request.path_ >> request.version_)) {
        throw std::runtime_error("Malformed request line");
    }

    std::transform(request.method_.begin(), request.method_.end(), request.method_.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    for (size_t i = 1; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        if (line.empty()) {
            continue;
        }
        auto colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        trim(key);
        trim(value);
        if (key.empty()) {
            continue;
        }
        request.headers_[toLower(key)] = value;
    }

    size_t contentLength = parseContentLength(request.headers_);
    requestLength = boundary->totalLength + contentLength;

    if (buffer.size() < requestLength) {
        return false;
    }

    request.body_.assign(buffer.begin() + static_cast<std::ptrdiff_t>(boundary->totalLength),
                         buffer.begin() + static_cast<std::ptrdiff_t>(boundary->totalLength + contentLength));

    return true;
}

void HttpRequest::reset() {
    method_.clear();
    path_.clear();
    version_.clear();
    headers_.clear();
    body_.clear();
}
