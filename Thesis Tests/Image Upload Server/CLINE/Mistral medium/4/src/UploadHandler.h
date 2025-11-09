#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H
#include <string>
#include <vector>

class UploadHandler {
public:
    std::string handle(const std::vector<char>& body, const std::string& contentType);
private:
    bool isValidImageExtension(const std::string& filename);
};
#endif // UPLOAD_HANDLER_H
