#include "src/UploadHandler.h"
#include <iostream>
#include <vector>

int main() {
    const std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string bodyStr;
    bodyStr += "--" + boundary + "\r\n";
    bodyStr += "Content-Disposition: form-data; name=\"text\"\r\n\r\n";
    bodyStr += "hello\r\n";
    bodyStr += "--" + boundary + "\r\n";
    bodyStr += "Content-Disposition: form-data; name=\"file\"; filename=\"test.png\"\r\n";
    bodyStr += "Content-Type: image/png\r\n\r\n";
    bodyStr += std::string("\x89PNG\r\n\x1a\n", 8);
    bodyStr += "PNGDATA";
    bodyStr += "\r\n--" + boundary + "--\r\n";

    std::vector<char> body(bodyStr.begin(), bodyStr.end());
    UploadHandler handler("Data");
    UploadResult result = handler.handle(body, "multipart/form-data; boundary=" + boundary);
    std::cout << result.statusCode << " " << result.message << std::endl;
    return 0;
}
