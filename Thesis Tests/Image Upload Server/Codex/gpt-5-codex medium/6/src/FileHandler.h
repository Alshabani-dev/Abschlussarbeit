#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <filesystem>
#include <string>

#include "HttpResponse.h"

class FileHandler {
public:
    FileHandler();

    void setPublicRoot(const std::filesystem::path& root);
    bool serve(const std::string& requestPath, HttpResponse& response) const;

private:
    std::filesystem::path publicRoot_;

    static std::string guessMimeType(const std::filesystem::path& path);
};

#endif // FILE_HANDLER_H
