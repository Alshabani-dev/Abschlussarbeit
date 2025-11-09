#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>

class HttpResponse;

class FileHandler {
public:
    explicit FileHandler(const std::string& publicDirectory);
    ~FileHandler();

    bool handle(const std::string& path, HttpResponse& response) const;

private:
    std::string publicDirectory_;
};

#endif  // FILE_HANDLER_H
