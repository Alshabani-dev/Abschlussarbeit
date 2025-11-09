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
    std::string extractFilename(const std::vector<char>& data, const std::string& boundary);
    std::vector<char> extractFileData(const std::vector<char>& data, const std::string& boundary);
    std::string saveFile(const std::string& filename, const std::vector<char>& data);
};

#endif // UPLOAD_HANDLER_H
