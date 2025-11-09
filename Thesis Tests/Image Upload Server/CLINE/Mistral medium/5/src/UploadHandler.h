#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include <string>
#include <vector>

class UploadHandler {
public:
    UploadHandler(const std::string& upload_dir = "Data");
    ~UploadHandler();

    std::string handleUpload(const std::vector<char>& body, const std::string& content_type);
    bool isValidImageExtension(const std::string& filename) const;

private:
    std::string upload_dir_;

    std::string generateFilename(const std::string& original_name) const;
    std::string extractBoundary(const std::string& content_type) const;
    std::string parseMultipartFormData(const std::vector<char>& body, const std::string& boundary);
    std::string saveFile(const std::string& filename, const std::vector<char>& data);
};

#endif // UPLOAD_HANDLER_H
