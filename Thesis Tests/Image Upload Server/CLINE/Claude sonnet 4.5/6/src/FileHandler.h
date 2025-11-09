#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <string>
#include <vector>
#include "HttpResponse.h"

class FileHandler {
public:
    FileHandler();
    HttpResponse serveFile(const std::string& path);
    
private:
    std::vector<char> readFile(const std::string& path);
    bool fileExists(const std::string& path);
    std::string getMimeType(const std::string& path);
};

#endif // FILEHANDLER_H
