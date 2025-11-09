#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>

class FileHandler {
public:
    FileHandler();
    std::string serveFile(const std::string& filepath);
    
private:
    std::string getMimeType(const std::string& filepath) const;
    std::string getFileExtension(const std::string& filepath) const;
};

#endif // FILE_HANDLER_H
