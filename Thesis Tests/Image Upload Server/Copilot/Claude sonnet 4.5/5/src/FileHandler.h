#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>

class FileHandler {
public:
    FileHandler(const std::string& rootDir);
    
    // Serve a file from the root directory
    // Returns true if file was found and loaded
    bool serve(const std::string& path, std::vector<char>& content, std::string& mimeType) const;
    
private:
    std::string rootDir_;
    
    bool fileExists(const std::string& path) const;
    bool readFile(const std::string& path, std::vector<char>& content) const;
};

#endif // FILE_HANDLER_H
