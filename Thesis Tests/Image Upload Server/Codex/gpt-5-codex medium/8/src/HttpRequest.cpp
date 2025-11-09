#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string trim(const std::string& str) {
    const auto start = str.find_first_not_of(" \r\n\t");
    if (start == std::string::npos) {
        return {};
    }
    const auto end = str.find_last_not_of(" \r\n\t");
    return str.substr(start, end - start + 1);
}
} // namespace

HttpRequest::Method HttpRequest::method() const {
    return method_;
}

const std::string& HttpRequest::uri() const {
    return uri_;
}

const std::unordered_map<std::string, std::string>& HttpRequest::headers() const {
    return headers_;
}

const std::vector<char>& HttpRequest::body() const {
    return body_;
}

std::string HttpRequest::headerValue(const std::string& key) const {
    auto it = headers_.find(toLower(key));
    if (it == headers_.end()) {
        return {};
    }
    return it->second;
}

bool HttpRequest::tryParse(std::vector<char>& buffer, HttpRequest& request) {
    const std::string data(buffer.begin(), buffer.end());

    const auto headerEnd = data.find("\r\n\r\n");
    size_t delimiterLen = 4;
    size_t headerBoundary = headerEnd;
    if (headerEnd == std::string::npos) {
        const auto altEnd = data.find("\n\n");
        if (altEnd == std::string::npos) {
            return false;
        }
        headerBoundary = altEnd;
        delimiterLen = 2;
    }

    std::string headersPart = data.substr(0, headerBoundary);
    std::istringstream stream(headersPart);
    std::string requestLine;
    if (!std::getline(stream, requestLine)) {
        return false;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream requestLineStream(requestLine);
    std::string methodToken;
    std::string uriToken;
    std::string versionToken;
    requestLineStream >> methodToken >> uriToken >> versionToken;
    if (methodToken.empty() || uriToken.empty()) {
        return false;
    }

    if (methodToken == "GET") {
        request.method_ = Method::Get;
    } else if (methodToken == "POST") {
        request.method_ = Method::Post;
    } else if (methodToken == "HEAD") {
        request.method_ = Method::Head;
    } else {
        request.method_ = Method::Unsupported;
    }

    request.uri_ = uriToken;
    request.headers_.clear();

    std::string headerLine;
    while (std::getline(stream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        const auto colon = headerLine.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string key = toLower(trim(headerLine.substr(0, colon)));
        std::string value = trim(headerLine.substr(colon + 1));
        request.headers_[key] = value;
    }

    const auto contentLengthStr = request.headerValue("content-length");
    size_t contentLength = 0;
    if (!contentLengthStr.empty()) {
        try {
            contentLength = static_cast<size_t>(std::stoul(contentLengthStr));
        } catch (const std::exception&) {
            contentLength = 0;
        }
    }

    const size_t totalNeeded = headerBoundary + delimiterLen + contentLength;
    if (buffer.size() < totalNeeded) {
        return false;
    }

    request.body_.assign(buffer.begin() + headerBoundary + delimiterLen, buffer.begin() + headerBoundary + delimiterLen + contentLength);
    buffer.erase(buffer.begin(), buffer.begin() + headerBoundary + delimiterLen + contentLength);
    return true;
}
