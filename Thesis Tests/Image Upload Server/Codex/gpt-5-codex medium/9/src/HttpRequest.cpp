#include "HttpRequest.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <string_view>

namespace {

void trimTrailingCarriageReturn(std::string& line) {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }
}

bool parseContentLength(const std::string& value, size_t& output) {
    output = 0;
    std::string_view view(value);
    // Trim leading spaces
    while (!view.empty() && std::isspace(static_cast<unsigned char>(view.front()))) {
        view.remove_prefix(1);
    }
    // Trim trailing spaces
    while (!view.empty() && std::isspace(static_cast<unsigned char>(view.back()))) {
        view.remove_suffix(1);
    }

    const char* begin = view.data();
    const char* end = begin + view.size();
    auto result = std::from_chars(begin, end, output);
    return result.ec == std::errc{} && result.ptr == end;
}

} // namespace

HttpRequest::ParseResult HttpRequest::parse(const std::vector<char>& rawData) {
    method_.clear();
    path_.clear();
    version_.clear();
    headers_.clear();
    body_.clear();
    error_.clear();
    complete_ = false;

    if (rawData.empty()) {
        return ParseResult::Incomplete;
    }

    const std::string delimiter1 = "\r\n\r\n";
    const std::string delimiter2 = "\n\n";

    auto it = std::search(rawData.begin(), rawData.end(), delimiter1.begin(), delimiter1.end());
    size_t headerEnd = std::distance(rawData.begin(), it);
    size_t delimiterLength = delimiter1.size();

    if (it == rawData.end()) {
        it = std::search(rawData.begin(), rawData.end(), delimiter2.begin(), delimiter2.end());
        headerEnd = std::distance(rawData.begin(), it);
        delimiterLength = delimiter2.size();
        if (it == rawData.end()) {
            return ParseResult::Incomplete;
        }
    }

    std::string headerBlock(rawData.begin(), rawData.begin() + headerEnd);
    std::istringstream stream(headerBlock);

    std::string requestLine;
    if (!std::getline(stream, requestLine)) {
        error_ = "Failed to read request line";
        return ParseResult::Error;
    }
    trimTrailingCarriageReturn(requestLine);
    std::istringstream requestLineStream(requestLine);
    if (!(requestLineStream >> method_ >> path_ >> version_)) {
        error_ = "Malformed request line";
        return ParseResult::Error;
    }

    std::string headerLine;
    while (std::getline(stream, headerLine)) {
        trimTrailingCarriageReturn(headerLine);
        if (headerLine.empty()) {
            continue;
        }
        auto colonPos = headerLine.find(':');
        if (colonPos == std::string::npos) {
            error_ = "Malformed header line";
            return ParseResult::Error;
        }
        std::string key = headerLine.substr(0, colonPos);
        std::string value = headerLine.substr(colonPos + 1);
        // Trim whitespace around value
        auto ltrim = [](std::string& s) {
            s.erase(s.begin(),
                    std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        };
        auto rtrim = [](std::string& s) {
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                    s.end());
        };
        ltrim(value);
        rtrim(value);
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        headers_[std::move(key)] = std::move(value);
    }

    const size_t bodyOffset = headerEnd + delimiterLength;
    const size_t availableBody = rawData.size() - bodyOffset;

    // GET/HEAD requests should not expect a body.
    if (method_ == "GET" || method_ == "HEAD") {
        complete_ = true;
        return ParseResult::Complete;
    }

    size_t contentLength = 0;
    auto itLength = headers_.find("content-length");
    if (itLength != headers_.end()) {
        if (!parseContentLength(itLength->second, contentLength)) {
            error_ = "Invalid Content-Length";
            return ParseResult::Error;
        }
    } else {
        // Without Content-Length, treat as zero-length body.
        contentLength = 0;
    }

    if (availableBody < contentLength) {
        return ParseResult::Incomplete;
    }

    body_.assign(rawData.begin() + static_cast<std::ptrdiff_t>(bodyOffset),
                 rawData.begin() + static_cast<std::ptrdiff_t>(bodyOffset + contentLength));

    complete_ = true;
    return ParseResult::Complete;
}

const std::string& HttpRequest::method() const {
    return method_;
}

const std::string& HttpRequest::path() const {
    return path_;
}

const std::string& HttpRequest::version() const {
    return version_;
}

const std::unordered_map<std::string, std::string>& HttpRequest::headers() const {
    return headers_;
}

const std::vector<char>& HttpRequest::body() const {
    return body_;
}

bool HttpRequest::isComplete() const {
    return complete_;
}

const std::string& HttpRequest::error() const {
    return error_;
}
