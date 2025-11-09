#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>
#include "HttpResponse.h"

class FileHandler {
public:
    FileHandler(const std::string& publicDir = "public");
    
    // Serve a file from the public directory
    HttpResponse serveFile(const std::string& path);
    
private:
    std::string publicDir_;
    
    // Read file contents (binary-safe)
    bool readFile(const std::string& filepath, std::vector<char>& content);
    
    // Get MIME type based on file extension
    std::string getMimeType(const std::string& filepath) const;
    
    // Check if path is safe (prevent directory traversal)
    bool isSafePath(const std::string& path) const;
};

#endif // FILE_HANDLER_H
