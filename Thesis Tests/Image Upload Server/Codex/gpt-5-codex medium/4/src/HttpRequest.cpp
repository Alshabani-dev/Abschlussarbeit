#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace {
void trimLeft(std::string& value) {
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

void trimRight(std::string& value) {
    value.erase(std::find_if(value.rbegin(), value.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                value.end());
}
} // namespace

HttpRequest::HttpRequest(const std::vector<char>& rawData) {
    std::string data(rawData.begin(), rawData.end());

    size_t delimiterLength = 4;
    size_t headerEnd = data.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        delimiterLength = 2;
        headerEnd = data.find("\n\n");
    }
    if (headerEnd == std::string::npos) {
        throw std::runtime_error("Malformed HTTP request: missing header terminator");
    }

    std::string headerSection = data.substr(0, headerEnd);
    body_.assign(rawData.begin() + headerEnd + delimiterLength, rawData.end());

    std::istringstream stream(headerSection);
    std::string requestLine;
    if (!std::getline(stream, requestLine)) {
        throw std::runtime_error("Malformed HTTP request: missing request line");
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream requestLineStream(requestLine);
    if (!(requestLineStream >> method_ >> path_ >> version_)) {
        throw std::runtime_error("Malformed HTTP request: invalid request line");
    }

    std::string headerLine;
    while (std::getline(stream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        if (headerLine.empty()) {
            continue;
        }
        const auto colonPos = headerLine.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        std::string key = headerLine.substr(0, colonPos);
        std::string value = headerLine.substr(colonPos + 1);
        trim(key);
        trim(value);
        headers_[toLower(key)] = value;
    }
}

const std::string& HttpRequest::getMethod() const {
    return method_;
}

const std::string& HttpRequest::getPath() const {
    return path_;
}

const std::string& HttpRequest::getVersion() const {
    return version_;
}

bool HttpRequest::hasHeader(const std::string& key) const {
    return headers_.find(toLower(key)) != headers_.end();
}

std::string HttpRequest::getHeader(const std::string& key) const {
    const auto it = headers_.find(toLower(key));
    if (it == headers_.end()) {
        return {};
    }
    return it->second;
}

const std::vector<char>& HttpRequest::getBody() const {
    return body_;
}

std::string HttpRequest::toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

void HttpRequest::trim(std::string& value) {
    trimLeft(value);
    trimRight(value);
}
