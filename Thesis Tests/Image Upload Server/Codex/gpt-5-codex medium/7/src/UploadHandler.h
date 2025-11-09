#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include "HttpRequest.h"
#include "HttpResponse.h"

#include <optional>
#include <string>

class UploadHandler {
public:
    explicit UploadHandler(std::string outputDirectory);

    HttpResponse handle(const HttpRequest& request) const;

private:
    static std::optional<std::string> extractBoundary(const std::string& contentTypeHeader);
    static std::optional<std::string> extractFilename(const std::string& partHeaders);
    static bool isValidImageExtension(const std::string& filename);
    static std::string sanitizeFilename(const std::string& filename);
    std::string outputDirectory_;
};

#endif // UPLOAD_HANDLER_H
