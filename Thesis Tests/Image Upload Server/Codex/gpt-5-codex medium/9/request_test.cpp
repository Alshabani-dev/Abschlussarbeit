#include "src/HttpRequest.h"
#include "src/UploadHandler.h"
#include <iostream>
#include <vector>

int main() {
    const std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string req;
    req += "POST / HTTP/1.1\r\n";
    req += "Host: localhost\r\n";
    req += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
    std::string body;
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"text\"\r\n\r\n";
    body += "hello\r\n";
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"test.png\"\r\n";
    body += "Content-Type: image/png\r\n\r\n";
    body += std::string("\x89PNG\r\n\x1a\n", 8);
    body += "PNGDATA";
    body += "\r\n--" + boundary + "--\r\n";
    req += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    req += "\r\n";
    req += body;

    std::vector<char> raw(req.begin(), req.end());
    HttpRequest request;
    auto result = request.parse(raw);
    if (result != HttpRequest::ParseResult::Complete) {
        std::cout << "not complete\n";
        return 0;
    }
    UploadHandler handler("Data");
    UploadResult res = handler.handle(request.body(), "multipart/form-data; boundary=" + boundary);
    std::cout << res.statusCode << " " << res.message << '\n';
    return 0;
}
