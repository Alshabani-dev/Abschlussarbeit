#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>

class FileHandler {
public:
    FileHandler();
    
    // Serve a file from the public directory
    // Returns status code and body via output parameters
    bool serveFile(const std::string& path, int& statusCode, 
                   std::string& statusMessage, std::string& contentType,
                   std::vector<char>& body);
    
private:
    std::string publicDir_;
    
    bool readFile(const std::string& fullPath, std::vector<char>& content);
    bool fileExists(const std::string& fullPath);
};

#endif // FILE_HANDLER_H
