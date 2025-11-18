#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "Utils.h"

HttpServer::HttpServer(int port) : port_(port) {}

void HttpServer::start(const std::function<std::string(const std::string &)> &handler) {
    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        close(serverFd);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverFd, 10) < 0) {
        close(serverFd);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Web interface ready on http://localhost:" << port_ << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            continue;
        }

        std::string request;
        char buffer[4096];
        ssize_t bytesRead;
        size_t expectedBody = 0;
        bool headerParsed = false;
        while ((bytesRead = recv(clientFd, buffer, sizeof(buffer), 0)) > 0) {
            request.append(buffer, buffer + bytesRead);
            auto headerEnd = request.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                if (!headerParsed) {
                    std::string headers = request.substr(0, headerEnd);
                    std::istringstream headerStream(headers);
                    std::string line;
                    while (std::getline(headerStream, line)) {
                        std::string trimmed = Utils::trim(line);
                        std::string lowered = Utils::toLower(trimmed);
                        if (lowered.rfind("content-length:", 0) == 0) {
                            size_t colon = line.find(':');
                            if (colon != std::string::npos) {
                                std::string value = Utils::trim(line.substr(colon + 1));
                                try {
                                    expectedBody = static_cast<size_t>(std::stoul(value));
                                } catch (...) {
                                    expectedBody = 0;
                                }
                                break;
                            }
                        }
                    }
                    headerParsed = true;
                }
                size_t bodySize = request.size() - (headerEnd + 4);
                if (bodySize >= expectedBody) {
                    break;
                }
            }
            if (bytesRead < static_cast<ssize_t>(sizeof(buffer))) {
                break;
            }
        }

        std::string response = handleRequest(request, handler);
        size_t totalSent = 0;
        while (totalSent < response.size()) {
            ssize_t sent = send(clientFd, response.c_str() + totalSent, response.size() - totalSent, 0);
            if (sent <= 0) {
                break;
            }
            totalSent += static_cast<size_t>(sent);
        }
        close(clientFd);
    }
}

std::string HttpServer::buildPage(const std::string &result) const {
    std::ostringstream html;
    html << R"HTML(<!doctype html>
<html>
<head>
<meta charset="utf-8" />
<title>Minimal SQL Interpreter</title>
<style>
body { font-family: sans-serif; margin: 2rem; }
textarea { width: 100%; height: 8rem; font-family: monospace; }
pre { background: #f5f5f5; padding: 1rem; border-radius: 4px; }
button { margin-top: 0.5rem; padding: 0.5rem 1rem; }
</style>
</head>
<body>
<h1>Minimal SQL Interpreter</h1>
<form method="POST">
<textarea name="sql" placeholder="Enter SQL statements here"></textarea>
<br/>
<button type="submit">Execute</button>
</form>
<h2>Result</h2>
<pre>)HTML";
    html << Utils::htmlEscape(result);
    html << "</pre>\n</body>\n</html>";
    return html.str();
}

std::string HttpServer::handleRequest(const std::string &request, const std::function<std::string(const std::string &)> &handler) const {
    if (request.rfind("POST", 0) == 0) {
        auto pos = request.find("\r\n\r\n");
        std::string body = pos == std::string::npos ? std::string() : request.substr(pos + 4);
        std::string sql;
        size_t start = 0;
        while (start < body.size()) {
            size_t eq = body.find('=', start);
            if (eq == std::string::npos) {
                break;
            }
            size_t amp = body.find('&', eq);
            std::string key = body.substr(start, eq - start);
            std::string value = body.substr(eq + 1, amp == std::string::npos ? std::string::npos : amp - eq - 1);
            if (key == "sql") {
                sql = Utils::urlDecode(value);
                break;
            }
            if (amp == std::string::npos) {
                break;
            }
            start = amp + 1;
        }
        std::string output = handler(sql);
        std::string page = buildPage(output);
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html; charset=utf-8\r\n";
        response << "Content-Length: " << page.size() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << page;
        return response.str();
    }

    std::string page = buildPage("");
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html; charset=utf-8\r\n";
    response << "Content-Length: " << page.size() << "\r\n";
    response << "Connection: close\r\n\r\n";
    response << page;
    return response.str();
}
