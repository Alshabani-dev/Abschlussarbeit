#include "UploadHandler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
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
        return "Error: Could not create Data directory";
    }

    // Generate timestamp for filename
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream timestamp;
    timestamp << std::put_time(&tm, "%Y%m%d_%H%M%S");

    // Create new filename with timestamp
    std::string newFilename = "Data/" + timestamp.str() + "_" + filename;

    // Save file
    std::ofstream file(newFilename, std::ios::binary);
    if (!file) {
        return "Error: Could not save file";
    }

    file.write(data.data(), data.size());
    return "File uploaded successfully: " + newFilename;
}

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Check if this is a multipart request
    if (contentType.find("multipart/form-data") == std::string::npos) {
        return "400 Bad Request: Not a multipart request";
    }

    // Extract boundary
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "400 Bad Request: No boundary found";
    }

    std::string boundary = "--" + contentType.substr(boundaryPos + 9);

    // Convert body to string for easier parsing
    std::string bodyStr(body.begin(), body.end());

    // Find filename in the multipart data
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
    std::vector<char> fileData;
    fileData.assign(body.begin() + fileDataStart, body.begin() + fileDataEnd);

    // Save file
    return saveFile(fileData, filename);
}
