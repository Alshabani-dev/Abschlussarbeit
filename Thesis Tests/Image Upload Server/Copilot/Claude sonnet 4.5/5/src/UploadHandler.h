#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    UploadHandler(const std::string& uploadDir);
    
    // Handle multipart/form-data upload
    // Returns status message (e.g., "200 OK" or "400 Bad Request: ...")
    std::string handle(const std::vector<char>& body, const std::string& contentType);
    
private:
    std::string uploadDir_;
    
    bool isValidImageExtension(const std::string& filename) const;
    std::string extractBoundary(const std::string& contentType) const;
    std::string extractFilename(const std::string& data, size_t start, size_t end) const;
    bool saveFile(const std::string& filename, const std::vector<char>& data);
    std::string getCurrentTimestamp() const;
};

#endif // UPLOAD_HANDLER_H
