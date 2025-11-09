#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

namespace {
void trimTrailingCarriageReturn(std::string& value) {
    if (!value.empty() && value.back() == '\r') {
        value.pop_back();
    }
}
} // namespace

bool HttpRequest::parse(const std::vector<char>& rawData) {
    headers_.clear();
    body_.clear();

    if (rawData.empty()) {
        return false;
    }

    std::size_t delimiterLength = 0;
    const auto boundaryOpt = findHeaderBoundary(rawData, delimiterLength);
    if (!boundaryOpt.has_value()) {
        return false;
    }

    const std::size_t headerEnd = boundaryOpt.value();
    std::string headerBlock(rawData.begin(), rawData.begin() + static_cast<std::ptrdiff_t>(headerEnd));
    std::istringstream headerStream(headerBlock);

    std::string requestLine;
    if (!std::getline(headerStream, requestLine)) {
        return false;
    }
    trimTrailingCarriageReturn(requestLine);

    std::istringstream requestLineStream(requestLine);
    if (!(requestLineStream >> method_ >> path_ >> version_)) {
        return false;
    }

    std::string headerLine;
    while (std::getline(headerStream, headerLine)) {
        trimTrailingCarriageReturn(headerLine);
        if (headerLine.empty()) {
            continue;
        }

        const auto colonPos = headerLine.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        std::string key = headerLine.substr(0, colonPos);
        std::string value = headerLine.substr(colonPos + 1);

        // Trim leading whitespace in the header value.
        const auto firstNonSpace = value.find_first_not_of(" \t");
        if (firstNonSpace != std::string::npos) {
            value = value.substr(firstNonSpace);
        } else {
            value.clear();
        }

        headers_[toLower(key)] = value;
    }

    const auto bodyBegin = rawData.begin() + static_cast<std::ptrdiff_t>(headerEnd + delimiterLength);
    body_.assign(bodyBegin, rawData.end());

    return true;
}

std::optional<std::string> HttpRequest::header(const std::string& key) const {
    const auto lowered = toLower(key);
    const auto it = headers_.find(lowered);
    if (it == headers_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::size_t> HttpRequest::findHeaderBoundary(const std::vector<char>& buffer, std::size_t& delimiterLength) {
    const std::string_view delimiterCRLF = "\r\n\r\n";
    const std::string_view delimiterLF = "\n\n";

    const auto searchWithDelimiter = [&](std::string_view delimiter) -> std::optional<std::size_t> {
        const auto it = std::search(buffer.begin(), buffer.end(), delimiter.begin(), delimiter.end());
        if (it == buffer.end()) {
            return std::nullopt;
        }
        const std::size_t position = static_cast<std::size_t>(std::distance(buffer.begin(), it));
        delimiterLength = delimiter.size();
        return position;
    };

    if (const auto position = searchWithDelimiter(delimiterCRLF); position.has_value()) {
        return position;
    }

    if (const auto position = searchWithDelimiter(delimiterLF); position.has_value()) {
        return position;
    }

    delimiterLength = 0;
    return std::nullopt;
}

std::string HttpRequest::toLower(const std::string& value) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowered;
}
