#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>

class FileHandler {
public:
    FileHandler();
    
    // Read file and return contents (binary-safe)
    bool readFile(const std::string& path, std::vector<char>& content);
    
    // Get MIME type based on file extension
    std::string getMimeType(const std::string& path) const;
    
private:
    std::string getExtension(const std::string& path) const;
};

#endif // FILE_HANDLER_H
