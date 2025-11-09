#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>

class HttpResponse;

class FileHandler {
public:
    explicit FileHandler(std::string publicRoot);

    bool serve(const std::string& urlPath, HttpResponse& response);

private:
    std::string publicRoot_;

    std::string resolvePath(const std::string& urlPath) const;
    static std::string detectMimeType(const std::string& path);
};

#endif // FILE_HANDLER_H
