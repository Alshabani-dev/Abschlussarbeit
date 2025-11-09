#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>

class FileHandler {
public:
    FileHandler();
    
    bool readFile(const std::string& path, std::vector<char>& content);
    bool writeFile(const std::string& path, const std::vector<char>& content);

private:
    bool fileExists(const std::string& path);
};

#endif // FILE_HANDLER_H
