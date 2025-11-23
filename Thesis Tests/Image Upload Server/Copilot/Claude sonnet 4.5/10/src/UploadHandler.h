#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    UploadHandler();
    
    // Handle multipart/form-data upload
    std::string handle(const std::vector<char>& body, const std::string& contentType);
    
private:
    bool isValidImageExtension(const std::string& filename);
    std::string extractBoundary(const std::string& contentType);
    std::string extractFilename(const std::string& data);
    bool saveFile(const std::string& filename, const std::vector<char>& data);
    std::string getExtension(const std::string& filename);
    std::string toLower(const std::string& str);
};

#endif // UPLOAD_HANDLER_H
