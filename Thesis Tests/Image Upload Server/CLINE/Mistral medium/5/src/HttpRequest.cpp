#include "HttpRequest.h"
#include <algorithm>
#include <sstream>
#include <cctype>

HttpRequest::HttpRequest(const std::string& raw_request) : valid_(true) {
    try {
        size_t header_end = findHeaderEnd(raw_request);
        if (header_end == std::string::npos) {
            valid_ = false;
            return;
        }

        // Parse request line
        size_t first_line_end = raw_request.find("\r\n");
        if (first_line_end == std::string::npos) {
            first_line_end = raw_request.find("\n");
            if (first_line_end == std::string::npos) {
                valid_ = false;
                return;
            }
        }

        std::string request_line = raw_request.substr(0, first_line_end);
        parseRequestLine(request_line);

        // Parse headers
        std::string headers_data = raw_request.substr(first_line_end + (raw_request[first_line_end + 1] == '\n' ? 2 : 1),
                                                      header_end - first_line_end - (raw_request[first_line_end + 1] == '\n' ? 2 : 1));
        parseHeaders(headers_data);

        // Parse body if present
        if (header_end + 4 < raw_request.size()) { // +4 for \r\n\r\n
            parseBody(raw_request);
        }
    } catch (...) {
        valid_ = false;
    }
}

HttpRequest::~HttpRequest() {}

const std::string& HttpRequest::getMethod() const { return method_; }
const std::string& HttpRequest::getPath() const { return path_; }
const std::string& HttpRequest::getVersion() const { return version_; }
const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return headers_; }
const std::vector<char>& HttpRequest::getBody() const { return body_; }

size_t HttpRequest::getContentLength() const {
    auto it = headers_.find("Content-Length");
    if (it != headers_.end()) {
        return std::stoul(it->second);
    }
    return 0;
}

bool HttpRequest::isValid() const { return valid_; }

std::string HttpRequest::getHeader(const std::string& name) const {
    auto it = headers_.find(name);
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

void HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    iss >> method_ >> path_ >> version_;
}

void HttpRequest::parseHeaders(const std::string& header_data) {
    std::istringstream iss(header_data);
    std::string header_line;

    while (std::getline(iss, header_line)) {
        if (header_line.empty()) continue;

        size_t colon_pos = header_line.find(':');
        if (colon_pos != std::string::npos) {
            std::string name = header_line.substr(0, colon_pos);
            std::string value = extractHeaderValue(header_line.substr(colon_pos + 1));

            // Convert to lowercase for case-insensitive comparison
            std::string lower_name;
            for (char c : name) {
                lower_name += tolower(c);
            }

            headers_[lower_name] = value;
        }
    }
}

void HttpRequest::parseBody(const std::string& raw_request) {
    size_t header_end = findHeaderEnd(raw_request);
    if (header_end != std::string::npos && header_end + 4 < raw_request.size()) {
        std::string body_str = raw_request.substr(header_end + 4);
        body_.assign(body_str.begin(), body_str.end());
    }
}

size_t HttpRequest::findHeaderEnd(const std::string& raw_request) const {
    size_t crlf_crlf = raw_request.find("\r\n\r\n");
    if (crlf_crlf != std::string::npos) {
        return crlf_crlf;
    }

    size_t lf_lf = raw_request.find("\n\n");
    if (lf_lf != std::string::npos) {
        return lf_lf;
    }

    return std::string::npos;
}

std::string HttpRequest::extractHeaderValue(const std::string& header_line) const {
    // Skip leading whitespace
    size_t start = header_line.find_first_not_of(" \t");
    if (start == std::string::npos) {
        return "";
    }

    // Find end of value (before any trailing whitespace)
    size_t end = header_line.find_last_not_of(" \t");
    if (end == std::string::npos || end < start) {
        return "";
    }

    return header_line.substr(start, end - start + 1);
}
