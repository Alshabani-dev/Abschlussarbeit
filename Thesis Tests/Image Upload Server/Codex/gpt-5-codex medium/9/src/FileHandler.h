#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>

class HttpResponse;

class FileHandler {
public:
    explicit FileHandler(std::string baseDir);
    bool serveFile(const std::string& path, HttpResponse& response) const;

private:
    std::string baseDir_;

    static std::string resolvePath(const std::string& baseDir, const std::string& path);
    static bool loadFile(const std::string& filePath, std::vector<char>& buffer);
    static std::string detectMimeType(const std::string& filePath);
};

#endif // FILE_HANDLER_H
