#include "UploadHandler.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }

    std::string ext = filename.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp";
}

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Check if this is a multipart form
    if (contentType.find("multipart/form-data") == std::string::npos) {
        return "400 Bad Request: Not a multipart form";
    }

    // Extract boundary
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "400 Bad Request: No boundary found";
    }

    std::string boundary = "--" + contentType.substr(boundaryPos + 9);

    // Convert body to string for parsing
    std::string bodyStr(body.begin(), body.end());

    // Find filename in the body
    size_t filenamePos = bodyStr.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return "400 Bad Request: No filename found";
    }

    size_t filenameStart = filenamePos + 10;
    size_t filenameEnd = bodyStr.find("\"", filenameStart);
    if (filenameEnd == std::string::npos) {
        return "400 Bad Request: Invalid filename format";
    }

    std::string filename = bodyStr.substr(filenameStart, filenameEnd - filenameStart);

    // Validate image extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid image format";
    }

    // Find file data start
    size_t fileDataStart = bodyStr.find("\r\n\r\n", filenameEnd);
    if (fileDataStart == std::string::npos) {
        fileDataStart = bodyStr.find("\n\n", filenameEnd);
        if (fileDataStart == std::string::npos) {
            return "400 Bad Request: No file data found";
        }
        fileDataStart += 2;
    } else {
        fileDataStart += 4;
    }

    // Find file data end (boundary)
    size_t fileDataEnd = bodyStr.find(boundary, fileDataStart);
    if (fileDataEnd == std::string::npos) {
        fileDataEnd = bodyStr.length();
    }

    // Extract file data
    std::vector<char> fileData(body.begin() + fileDataStart, body.begin() + fileDataEnd);

    // Create Data directory if it doesn't exist
    if (mkdir("Data", 0777) != 0 && errno != EEXIST) {
        return "500 Internal Server Error: Could not create Data directory";
    }

    // Generate unique filename with timestamp
    std::time_t now = std::time(nullptr);
    std::string timestamp = std::to_string(now);
    std::string uniqueFilename = "Data/" + timestamp + "_" + filename;

    // Save file
    std::ofstream outFile(uniqueFilename, std::ios::binary);
    if (!outFile) {
        return "500 Internal Server Error: Could not save file";
    }

    outFile.write(fileData.data(), fileData.size());
    outFile.close();

    return "200 OK: File uploaded successfully";
}
