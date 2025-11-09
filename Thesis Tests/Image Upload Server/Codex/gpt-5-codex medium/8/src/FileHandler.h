#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <filesystem>
#include <string>
#include <vector>

class FileHandler {
public:
    explicit FileHandler(std::string publicRoot);

    bool readFile(const std::string& path, std::vector<char>& out);
    std::string detectMimeType(const std::string& path) const;

private:
    std::filesystem::path publicRoot_;
};

#endif // FILE_HANDLER_H
