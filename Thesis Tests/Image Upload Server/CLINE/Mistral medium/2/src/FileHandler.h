#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H
#include <string>

class FileHandler {
public:
    std::string readFile(const std::string& filePath);
    std::string getMimeType(const std::string& filePath);
};
#endif // FILE_HANDLER_H
