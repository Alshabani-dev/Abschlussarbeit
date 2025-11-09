#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>

class FileHandler {
public:
    FileHandler(const std::string& root_dir = "public");
    ~FileHandler();

    std::vector<char> readFile(const std::string& path);
    bool fileExists(const std::string& path) const;
    std::string getMimeType(const std::string& path) const;
    std::string getContentType(const std::string& path) const;

private:
    std::string root_dir_;

    std::string sanitizePath(const std::string& path) const;
    bool isDirectory(const std::string& path) const;
};

#endif // FILE_HANDLER_H
