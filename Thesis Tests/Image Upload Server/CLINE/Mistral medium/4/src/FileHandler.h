#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include <string>

class FileHandler {
public:
    std::string handle(const std::string& path);
    std::string getMimeType(const std::string& fileExtension);
private:
    bool fileExists(const std::string& path);
};
#endif // FILEHANDLER_H
