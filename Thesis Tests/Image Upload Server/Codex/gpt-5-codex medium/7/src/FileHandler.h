#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "HttpResponse.h"

#include <optional>
#include <string>

class FileHandler {
public:
    explicit FileHandler(std::string publicDirectory);

    HttpResponse serve(const std::string& requestPath) const;

private:
    [[nodiscard]] std::optional<std::string> resolvePath(const std::string& requestPath) const;
    [[nodiscard]] static std::string guessMimeType(const std::string& filePath);

    std::string publicDirectory_;
};

#endif // FILE_HANDLER_H
