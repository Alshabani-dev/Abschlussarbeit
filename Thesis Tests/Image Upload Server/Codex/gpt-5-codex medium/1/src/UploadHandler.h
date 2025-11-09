#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

#include "HttpResponse.h"

class UploadHandler {
public:
    explicit UploadHandler(std::string dataDir);

    HttpResponse handle(const std::vector<char>& body, const std::string& contentType);

private:
    std::string dataDir_;

    static bool isValidImageExtension(const std::string& filename);
    static std::string extractBoundary(const std::string& contentType);
    static std::string extractFilename(const std::string& partHeaders);
    std::string persistFile(const std::vector<char>& data, const std::string& originalName) const;
};

#endif // UPLOAD_HANDLER_H
