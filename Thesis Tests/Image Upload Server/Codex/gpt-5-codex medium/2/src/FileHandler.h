#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpResponse;

class FileHandler {
public:
    FileHandler();

    HttpResponse serveFile(const std::string& root, const std::string& urlPath);

private:
    std::unordered_map<std::string, std::string> mimeTypes_;

    std::string resolvePath(const std::string& root, const std::string& urlPath) const;
    std::string detectMimeType(const std::string& extension) const;
};

#endif // FILE_HANDLER_H
