#include "HttpResponse.h"
#include <sstream>
#include <ctime>
#include <iomanip>

HttpResponse::HttpResponse(int status_code)
    : status_code_(status_code), binary_body_(false) {
    // Set default headers
    setHeader("Server", "SimpleCppWebServer");
    setHeader("Connection", "close");
}

HttpResponse::~HttpResponse() {}

void HttpResponse::setStatus(int status_code) {
    status_code_ = status_code;
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    headers_[name] = value;
}

void HttpResponse::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
    binary_body_ = false;
    // Set Content-Length automatically
    setHeader("Content-Length", std::to_string(body_.size()));
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
    binary_body_ = true;
    // Set Content-Length automatically
    setHeader("Content-Length", std::to_string(body_.size()));
}

std::string HttpResponse::toString() const {
    std::ostringstream oss;

    // Status line
    oss << "HTTP/1.1 " << status_code_ << " " << getStatusText() << "\r\n";

    // Add Date header if not already set
    if (headers_.find("Date") == headers_.end()) {
        std::time_t now = std::time(nullptr);
        std::tm tm = *std::gmtime(&now);
        std::ostringstream date_oss;
        date_oss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
        oss << "Date: " << date_oss.str() << "\r\n";
    }

    // Headers
    for (const auto& header : headers_) {
        oss << header.first << ": " << header.second << "\r\n";
    }

    // Empty line to separate headers from body
    oss << "\r\n";

    return oss.str();
}

const std::vector<char>& HttpResponse::getBinaryBody() const {
    return body_;
}

std::string HttpResponse::getStatusText() const {
    switch (status_code_) {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown Status";
    }
}

std::string HttpResponse::getDefaultHeader(const std::string& name) const {
    static const std::map<std::string, std::string> default_headers = {
        {"Server", "SimpleCppWebServer"},
        {"Connection", "close"},
        {"Date", []() {
            std::time_t now = std::time(nullptr);
            std::tm tm = *std::gmtime(&now);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
            return oss.str();
        }()}
    };

    auto it = default_headers.find(name);
    if (it != default_headers.end()) {
        return it->second;
    }
    return "";
}
