#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <filesystem>
#include <string>
#include <vector>

struct UploadResult {
    int statusCode;
    std::string message;
    std::string storedFilename;
};

class UploadHandler {
public:
    UploadHandler();

    void setDataRoot(const std::filesystem::path& root);
    UploadResult handle(const std::vector<char>& body, const std::string& contentType) const;

private:
    std::filesystem::path dataRoot_;

    static bool isValidImageExtension(const std::string& filename);
    static std::string trim(const std::string& value);
    static std::string extractBoundary(const std::string& contentType);
    static std::string extractFilename(const std::string& dispositionLine);
    static std::string sanitizeFilename(const std::string& filename);
};

#endif // UPLOAD_HANDLER_H
