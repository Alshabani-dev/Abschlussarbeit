#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

size_t findDelimiter(const std::vector<char>& data, const std::string& delimiter) {
    if (data.size() < delimiter.size()) {
        return std::string::npos;
    }
    for (size_t i = 0; i <= data.size() - delimiter.size(); ++i) {
        if (std::equal(delimiter.begin(), delimiter.end(), data.begin() + static_cast<std::ptrdiff_t>(i))) {
            return i;
        }
    }
    return std::string::npos;
}

std::string trim(const std::string& input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    return input.substr(start, end - start);
}

} // namespace

bool HttpRequest::parse(const std::vector<char>& raw) {
    method_.clear();
    path_.clear();
    version_.clear();
    headers_.clear();
    body_.clear();

    const std::string delimiterCRLF = "\r\n\r\n";
    const std::string delimiterLF = "\n\n";

    size_t delimiterPos = findDelimiter(raw, delimiterCRLF);
    size_t delimiterLen = delimiterCRLF.length();

    if (delimiterPos == std::string::npos) {
        delimiterPos = findDelimiter(raw, delimiterLF);
        delimiterLen = delimiterLF.length();
    }

    if (delimiterPos == std::string::npos) {
        return false;
    }

    std::string headerSection(raw.data(), delimiterPos);
    std::string bodySection(raw.data() + static_cast<std::ptrdiff_t>(delimiterPos + delimiterLen),
                            raw.size() - delimiterPos - delimiterLen);

    std::istringstream stream(headerSection);
    std::string requestLine;
    if (!std::getline(stream, requestLine)) {
        return false;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream requestLineStream(requestLine);
    if (!(requestLineStream >> method_ >> path_ >> version_)) {
        return false;
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, colonPos));
        const std::string value = trim(line.substr(colonPos + 1));
        if (!key.empty()) {
            headers_[normalizeKey(key)] = value;
        }
    }

    body_.assign(bodySection.begin(), bodySection.end());
    return true;
}

bool HttpRequest::hasHeader(const std::string& key) const {
    return headers_.find(normalizeKey(key)) != headers_.end();
}

std::string HttpRequest::headerValue(const std::string& key) const {
    auto it = headers_.find(normalizeKey(key));
    if (it != headers_.end()) {
        return it->second;
    }
    return {};
}

std::string HttpRequest::normalizeKey(const std::string& rawKey) {
    std::string normalized = rawKey;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return normalized;
}
