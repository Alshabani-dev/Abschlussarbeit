#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    std::string handle(const std::string& body, const std::string& contentType);
private:
    bool isValidImageExtension(const std::string& filename);
    std::string extractFilename(const std::string& contentType, const std::string& body);
    std::string extractFileData(const std::string& boundary, const std::string& body);
    std::string generateUniqueFilename(const std::string& originalFilename);
};

#endif // UPLOAD_HANDLER_H
