#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>
#include "HttpResponse.h"

class UploadHandler {
public:
    UploadHandler(const std::string& uploadDir = "Data");
    
    // Handle file upload
    HttpResponse handle(const std::vector<char>& body, const std::string& contentType);
    
private:
    std::string uploadDir_;
    
    // Validate image file extension
    bool isValidImageExtension(const std::string& filename) const;
    
    // Extract filename from multipart data
    std::string extractFilename(const std::string& data) const;
    
    // Save file to disk (binary-safe)
    bool saveFile(const std::string& filename, const std::vector<char>& data);
    
    // Generate unique filename with timestamp
    std::string generateUniqueFilename(const std::string& originalName) const;
};

#endif // UPLOAD_HANDLER_H
