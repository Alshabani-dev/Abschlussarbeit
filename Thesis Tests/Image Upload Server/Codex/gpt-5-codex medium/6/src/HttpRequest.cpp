#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {
constexpr char CR = '\r';
constexpr char LF = '\n';

std::string normalizeLineEndings(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        char ch = input[i];
        if (ch == CR) {
            if (i + 1 < input.size() && input[i + 1] == LF) {
                output.push_back('\n');
                ++i;
            } else {
                output.push_back('\n');
            }
        } else {
            output.push_back(ch);
        }
    }
    return output;
}
} // namespace

bool HttpRequest::tryParse(const std::vector<char>& buffer, HttpRequest& request, std::size_t& consumedBytes) {
    consumedBytes = 0;
    if (buffer.empty()) {
        return false;
    }

    std::string data(buffer.begin(), buffer.end());

    std::size_t headerEnd = data.find("\r\n\r\n");
    std::size_t delimiterLen = 4;
    const std::size_t altHeaderEnd = data.find("\n\n");
    if (headerEnd == std::string::npos || (altHeaderEnd != std::string::npos && altHeaderEnd < headerEnd)) {
        headerEnd = altHeaderEnd;
        delimiterLen = 2;
    }

    if (headerEnd == std::string::npos) {
        return false;
    }

    const std::size_t bodyStart = headerEnd + delimiterLen;

    const std::string headerPart = data.substr(0, headerEnd);
    std::istringstream headerStream(normalizeLineEndings(headerPart));

    std::string requestLine;
    if (!std::getline(headerStream, requestLine) || requestLine.empty()) {
        return false;
    }

    std::istringstream requestLineStream(requestLine);
    std::string method;
    std::string path;
    std::string version;
    if (!(requestLineStream >> method >> path >> version)) {
        return false;
    }

    std::unordered_map<std::string, std::string> headers;
    std::string line;
    while (std::getline(headerStream, line)) {
        if (line.empty()) {
            continue;
        }
        const auto colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), value.end());

        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        headers[key] = value;
    }

    std::size_t contentLength = 0;
    if (const auto it = headers.find("content-length"); it != headers.end()) {
        try {
            contentLength = static_cast<std::size_t>(std::stoul(it->second));
        } catch (const std::exception&) {
            return false;
        }
    }

    if (buffer.size() < bodyStart + contentLength) {
        return false;
    }

    HttpRequest parsed;
    parsed.method_ = method;
    parsed.path_ = path;
    parsed.version_ = version;
    parsed.headers_ = std::move(headers);
    parsed.body_.assign(buffer.begin() + static_cast<std::ptrdiff_t>(bodyStart),
                        buffer.begin() + static_cast<std::ptrdiff_t>(bodyStart + contentLength));

    request = std::move(parsed);
    consumedBytes = bodyStart + contentLength;
    return true;
}

bool HttpRequest::hasHeader(const std::string& key) const {
    return headers_.find(toLower(key)) != headers_.end();
}

std::string HttpRequest::headerValue(const std::string& key) const {
    const auto it = headers_.find(toLower(key));
    if (it != headers_.end()) {
        return it->second;
    }
    return {};
}

std::string HttpRequest::toLower(const std::string& value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}
