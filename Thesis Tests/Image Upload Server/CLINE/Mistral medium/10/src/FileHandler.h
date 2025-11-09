#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>

class FileHandler {
public:
    std::string handle(const std::string& path);
    std::string getMimeType(const std::string& fileExtension);
};

#endif // FILE_HANDLER_H
