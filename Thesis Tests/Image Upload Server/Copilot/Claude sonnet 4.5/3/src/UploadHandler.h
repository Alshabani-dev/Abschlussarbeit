#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    UploadHandler();
    
    // Handle file upload from multipart/form-data
    // Returns status code and message
    std::string handle(const std::vector<char>& body, const std::string& contentType);
    
private:
    std::string dataDir_;
    
    bool isValidImageExtension(const std::string& filename);
    std::string extractBoundary(const std::string& contentType);
    std::string extractFilename(const std::string& data);
    bool saveFile(const std::string& filename, const std::vector<char>& content);
    std::string generateTimestampFilename(const std::string& originalName);
};

#endif // UPLOAD_HANDLER_H
