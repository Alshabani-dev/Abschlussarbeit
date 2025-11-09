#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    struct Result {
        bool success = false;
        int statusCode = 500;
        std::string reason;
        std::string message;
        std::string storedFilename;
    };

    Result handle(const std::vector<char>& body, const std::string& contentType) const;

private:
    static bool isValidImageExtension(const std::string& filename);
    static std::string extractBoundary(const std::string& contentType);
    static std::string sanitizeFilename(const std::string& filename);
    static std::string timestampedName(const std::string& originalName);
};

#endif // UPLOAD_HANDLER_H
