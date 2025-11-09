#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <filesystem>
#include <string>
#include <vector>

struct UploadResult {
    bool success;
    std::string message;
};

class UploadHandler {
public:
    explicit UploadHandler(std::string storageDir);

    UploadResult handle(const std::vector<char>& body, const std::string& contentType);

private:
    std::filesystem::path storageDir_;
    std::string storageDirString_;

    static std::string trim(const std::string& value);
    static std::string toLower(std::string value);
    std::string extractBoundary(const std::string& contentType) const;
    static bool isValidImageExtension(const std::string& filename);
};

#endif // UPLOAD_HANDLER_H
