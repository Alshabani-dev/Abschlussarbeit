#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    explicit UploadHandler(const std::string& dataDirectory);
    ~UploadHandler();

    struct Result {
        bool success;
        std::string message;
        std::string storedFilename;
    };

    Result handle(const std::vector<char>& body, const std::string& contentType) const;

private:
    std::string dataDirectory_;

    bool isValidImageExtension(const std::string& filename) const;
    std::string extractBoundary(const std::string& contentType) const;
    std::string sanitizeFilename(const std::string& filename) const;
};

#endif // UPLOAD_HANDLER_H
