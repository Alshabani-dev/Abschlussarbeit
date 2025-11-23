#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    // Handle multipart/form-data upload
    // Returns status message (e.g., "200 OK" or "400 Bad Request: reason")
    std::string handle(const std::vector<char>& body, const std::string& contentType);
    
private:
    // Validate image file extensions
    bool isValidImageExtension(const std::string& filename);
    
    // Extract boundary from Content-Type header
    std::string extractBoundary(const std::string& contentType);
    
    // Save file to Data directory with timestamp
    bool saveFile(const std::string& filename, const std::vector<char>& data);
};

#endif // UPLOAD_HANDLER_H
