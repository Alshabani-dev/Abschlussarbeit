#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    UploadHandler();
    std::string handle(const std::vector<char>& body, const std::string& contentType);
    
private:
    bool isValidImageExtension(const std::string& filename) const;
    std::string extractBoundary(const std::string& contentType) const;
    std::string extractFilename(const std::string& data) const;
    std::string getFileExtension(const std::string& filename) const;
    std::string generateTimestamp() const;
    bool saveFile(const std::string& filepath, const std::vector<char>& data);
};

#endif // UPLOAD_HANDLER_H
