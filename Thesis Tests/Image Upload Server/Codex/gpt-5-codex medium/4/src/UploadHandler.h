#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    struct Result {
        bool success;
        std::string message;
    };

    Result handle(const std::vector<char>& body, const std::string& contentType) const;

private:
    static bool isValidImageExtension(const std::string& filename);
    static std::string sanitizeFilename(std::string filename);
    static std::string generateTargetPath(const std::string& sanitizedName);
};

#endif // UPLOAD_HANDLER_H
