#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>

#include "HttpResponse.h"

class FileHandler {
public:
    explicit FileHandler(std::string publicRoot);

    HttpResponse serve(const std::string& requestPath) const;

private:
    std::string publicRoot_;

    std::string resolvePath(const std::string& requestPath) const;
    static std::string mimeTypeForExtension(const std::string& extension);
};

#endif // FILE_HANDLER_H
