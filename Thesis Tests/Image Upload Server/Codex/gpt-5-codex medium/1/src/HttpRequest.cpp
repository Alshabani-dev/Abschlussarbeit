#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace {
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    return str.substr(start, end - start);
}
} // namespace

HttpRequest HttpRequest::parse(const std::vector<char>& data) {
    HttpRequest request;

    const std::string raw(data.begin(), data.end());
    const std::string delimiter = "\r\n\r\n";
    const std::string altDelimiter = "\n\n";

    size_t headerEnd = raw.find(delimiter);
    size_t delimiterLength = delimiter.size();

    if (headerEnd == std::string::npos) {
        headerEnd = raw.find(altDelimiter);
        delimiterLength = altDelimiter.size();
    }

    if (headerEnd == std::string::npos) {
        throw std::runtime_error("Malformed HTTP request: missing header terminator");
    }

    std::string headerPart = raw.substr(0, headerEnd);
    std::istringstream headerStream(headerPart);
    std::string requestLine;
    if (!std::getline(headerStream, requestLine)) {
        throw std::runtime_error("Malformed HTTP request: missing request line");
    }

    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream requestLineStream(requestLine);
    if (!(requestLineStream >> request.method_ >> request.path_ >> request.version_)) {
        throw std::runtime_error("Malformed HTTP request: invalid request line");
    }

    std::string headerLine;
    while (std::getline(headerStream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        const auto separator = headerLine.find(':');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = toLower(trim(headerLine.substr(0, separator)));
        const std::string value = trim(headerLine.substr(separator + 1));
        request.headers_.emplace(key, value);
    }

    const size_t bodyStart = headerEnd + delimiterLength;
    if (bodyStart <= raw.size()) {
        request.body_ = std::vector<char>(data.begin() + static_cast<std::ptrdiff_t>(bodyStart), data.end());
    }

    return request;
}

bool HttpRequest::headerEquals(const std::string& key, const std::string& value) const {
    const std::string loweredKey = toLower(key);
    const auto it = headers_.find(loweredKey);
    if (it == headers_.end()) {
        return false;
    }
    return toLower(it->second) == toLower(value);
}

std::string HttpRequest::headerValue(const std::string& key) const {
    const std::string loweredKey = toLower(key);
    const auto it = headers_.find(loweredKey);
    if (it == headers_.end()) {
        return {};
    }
    return it->second;
}
