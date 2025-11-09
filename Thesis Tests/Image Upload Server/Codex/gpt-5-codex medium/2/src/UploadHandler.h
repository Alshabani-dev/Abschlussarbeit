#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class HttpResponse;

class UploadHandler {
public:
    UploadHandler();

    HttpResponse handle(const std::vector<char>& body, const std::string& contentType);

private:
    std::string uploadsDir_;

    bool isValidImageExtension(const std::string& filename) const;
    std::string extractBoundary(const std::string& contentType) const;
    HttpResponse badRequest(const std::string& message) const;
    HttpResponse serverError(const std::string& message) const;
};

#endif // UPLOAD_HANDLER_H
