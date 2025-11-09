#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include <string>
#include <vector>

class FileHandler {
public:
    std::vector<char> readFile(const std::string& path);
    std::string getMimeType(const std::string& path);
    bool fileExists(const std::string& path);
};
#endif // FILEHANDLER_H
