#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>

class FileHandler {
public:
    FileHandler(const std::string& public_dir);
    
    // Read a file from the public directory
    // Returns empty vector if file not found or error
    std::vector<char> readFile(const std::string& path);
    
    // Check if a file exists
    bool fileExists(const std::string& path);
    
private:
    std::string public_dir_;
    
    // Sanitize path to prevent directory traversal attacks
    std::string sanitizePath(const std::string& path);
};

#endif // FILE_HANDLER_H
