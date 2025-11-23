#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    UploadHandler();
    
    std::string handle(const std::vector<char>& body, const std::string& contentType);

private:
    bool isValidImageExtension(const std::string& filename);
    std::string extractBoundary(const std::string& contentType);
    std::string extractFilename(const std::string& data, size_t start, size_t end);
    std::string generateTimestamp();
};

#endif // UPLOAD_HANDLER_H
