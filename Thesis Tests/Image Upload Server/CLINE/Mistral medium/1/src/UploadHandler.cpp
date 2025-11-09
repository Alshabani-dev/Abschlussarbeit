#include "UploadHandler.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }

    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp";
}

std::string UploadHandler::saveFile(const std::vector<char>& data, const std::string& filename) {
    // Create Data directory if it doesn't exist
    if (mkdir("Data", 0755) != 0 && errno != EEXIST) {
        return "500 Internal Server Error: Could not create Data directory";
    }

    // Generate timestamp for filename
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm);

    // Extract file extension
    size_t dotPos = filename.find_last_of('.');
    std::string ext = (dotPos != std::string::npos) ? filename.substr(dotPos) : "";

    // Create new filename
    std::string newFilename = "Data/" + std::string(timestamp) + ext;

    // Save file
    std::ofstream file(newFilename, std::ios::binary);
    if (!file) {
        return "500 Internal Server Error: Could not save file";
    }

    file.write(data.data(), data.size());
    return "200 OK: File uploaded successfully to " + newFilename;
}

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Check if this is a multipart form
    if (contentType.find("multipart/form-data") == std::string::npos) {
        return "400 Bad Request: Not a multipart form";
    }

    // Find boundary
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "400 Bad Request: No boundary found";
    }

    std::string boundary = "--" + contentType.substr(boundaryPos + 9);

    // Find filename in the body
    std::string bodyStr(body.begin(), body.end());
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

    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type. Only JPG, PNG, GIF, BMP allowed.";
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

    // Save file
    return saveFile(fileData, filename);
}
