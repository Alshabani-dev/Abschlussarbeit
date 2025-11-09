#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H
#include <string>

class FileHandler {
public:
    std::string handle(const std::string& path);
private:
    std::string getMimeType(const std::string& path);
    bool fileExists(const std::string& path);
};
#endif // FILE_HANDLER_H
