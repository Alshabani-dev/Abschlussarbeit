#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>
#include "HttpResponse.h"

class FileHandler {
public:
    FileHandler(const std::string& publicDir = "public");
    
    HttpResponse handle(const std::string& path);
    
private:
    std::string publicDir_;
    
    bool fileExists(const std::string& filepath) const;
    std::vector<char> readFile(const std::string& filepath) const;
    std::string getMimeType(const std::string& filepath) const;
    std::string getFileExtension(const std::string& filepath) const;
};

#endif // FILE_HANDLER_H
