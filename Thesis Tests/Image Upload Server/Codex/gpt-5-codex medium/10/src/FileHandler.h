#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>

class FileHandler {
public:
    explicit FileHandler(std::string rootDirectory = "public");

    bool readFile(const std::string& requestPath, std::vector<char>& data, std::string& mimeType) const;

private:
    std::string rootDirectory_;

    static std::string sanitize(const std::string& requestPath);
    static std::string guessMime(const std::string& path);
};

#endif // FILE_HANDLER_H
