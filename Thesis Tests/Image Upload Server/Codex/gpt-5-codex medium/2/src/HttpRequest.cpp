#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace {

std::size_t findHeaderDelimiter(const std::vector<char>& buffer, std::size_t& delimiterLength) {
    std::string view(buffer.begin(), buffer.end());
    std::size_t pos = view.find("\r\n\r\n");
    if (pos != std::string::npos) {
        delimiterLength = 4;
        return pos;
    }
    pos = view.find("\n\n");
    if (pos != std::string::npos) {
        delimiterLength = 2;
        return pos;
    }
    delimiterLength = 0;
    return std::string::npos;
}

std::string trim(const std::string& input) {
    std::size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    std::size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    return input.substr(start, end - start);
}

std::vector<std::string> splitHeaderLines(const std::string& headers) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start < headers.size()) {
        std::size_t end = headers.find('\n', start);
        if (end == std::string::npos) {
            end = headers.size();
        }
        std::string line = headers.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
        start = end + 1;
    }
    return lines;
}

std::string lowercase(const std::string& value) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowered;
}

} // namespace

HttpRequest HttpRequest::parse(const std::vector<char>& raw) {
    if (raw.empty()) {
        throw std::runtime_error("Empty request");
    }

    std::size_t delimiterLength = 0;
    std::size_t headerPos = findHeaderDelimiter(raw, delimiterLength);
    if (headerPos == std::string::npos) {
        throw std::runtime_error("Header delimiter not found");
    }

    std::string headerBlock(raw.begin(), raw.begin() + static_cast<long>(headerPos));
    std::vector<std::string> lines = splitHeaderLines(headerBlock);
    if (lines.empty()) {
        throw std::runtime_error("Missing request line");
    }

    HttpRequest request;
    {
        std::istringstream lineStream(lines.front());
        if (!(lineStream >> request.method_ >> request.path_ >> request.version_)) {
            throw std::runtime_error("Malformed request line");
        }
    }

    for (std::size_t i = 1; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        if (line.empty()) {
            continue;
        }

        std::size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = lowercase(trim(line.substr(0, colon)));
        std::string value = trim(line.substr(colon + 1));
        request.headers_[key] = value;
    }

    request.body_.assign(raw.begin() + static_cast<long>(headerPos + delimiterLength), raw.end());
    return request;
}

const std::string& HttpRequest::method() const noexcept {
    return method_;
}

const std::string& HttpRequest::path() const noexcept {
    return path_;
}

const std::string& HttpRequest::version() const noexcept {
    return version_;
}

const std::unordered_map<std::string, std::string>& HttpRequest::headers() const noexcept {
    return headers_;
}

const std::vector<char>& HttpRequest::body() const noexcept {
    return body_;
}

bool HttpRequest::hasHeader(const std::string& key) const {
    std::string lowered = lowercase(key);
    return headers_.find(lowered) != headers_.end();
}

std::string HttpRequest::headerValue(const std::string& key) const {
    std::string lowered = lowercase(key);
    auto it = headers_.find(lowered);
    if (it == headers_.end()) {
        return {};
    }
    return it->second;
}
