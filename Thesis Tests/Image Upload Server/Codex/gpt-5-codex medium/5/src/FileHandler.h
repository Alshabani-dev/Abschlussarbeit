#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "HttpResponse.h"

#include <filesystem>
#include <string>

class FileHandler {
public:
    explicit FileHandler(std::string rootDir);

    bool serve(const std::string& requestPath, HttpResponse& response);

private:
    std::filesystem::path rootDir_;
    std::string rootDirString_;

    std::filesystem::path resolvePath(const std::string& requestPath) const;
    static std::string detectMimeType(const std::string& path);
};

#endif // FILE_HANDLER_H
