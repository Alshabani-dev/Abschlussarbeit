#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>

class FileHandler {
public:
    FileHandler();
    
    bool fileExists(const std::string& path) const;
    std::vector<char> readFile(const std::string& path) const;
    std::string getMimeType(const std::string& path) const;
    
private:
    std::string getExtension(const std::string& path) const;
};

#endif // FILE_HANDLER_H
