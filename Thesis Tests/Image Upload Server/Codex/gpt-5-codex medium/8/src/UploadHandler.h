#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    explicit UploadHandler(std::string dataDir);

    struct Result {
        bool success{false};
        std::string message;
    };

    Result handle(const std::vector<char>& body, const std::string& contentType);

private:
    std::string dataDir_;

    static std::string parseBoundary(const std::string& contentType);
    static std::string extractFilename(const std::string& headers);
    bool isValidImageExtension(const std::string& filename) const;
    std::string writeFile(const std::string& filename, const std::vector<char>& data) const;
};

#endif // UPLOAD_HANDLER_H
