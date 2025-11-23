#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    std::string handle(const std::vector<char>& body, const std::string& contentType);
    
private:
    bool isValidImageExtension(const std::string& filename);
    std::string extractFilename(const std::string& contentDisposition);
    std::string getFileExtension(const std::string& filename);
    bool saveFile(const std::string& filepath, const std::vector<char>& data);
    std::string generateTimestamp();
};

#endif // UPLOAD_HANDLER_H
