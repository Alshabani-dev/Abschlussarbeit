#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

struct UploadResult {
    int statusCode;
    std::string message;
};

class UploadHandler {
public:
    explicit UploadHandler(std::string outputDir);
    UploadResult handle(const std::vector<char>& body, const std::string& contentType);

private:
    std::string outputDir_;

    bool parseMultipart(const std::vector<char>& body,
                        const std::string& boundary,
                        std::string& filename,
                        std::vector<char>& fileData) const;
    bool isValidImageExtension(const std::string& filename) const;
    std::string storeFile(const std::string& filename, const std::vector<char>& data) const;
    static std::string extractBoundary(const std::string& contentType);
};

#endif // UPLOAD_HANDLER_H
