#include "UploadHandler.h"
#include <fstream>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }

    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp";
}

std::string UploadHandler::extractFilename(const std::string& contentType, const std::string& body) {
    // Extract boundary from content type
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "";
    }

    std::string boundary = contentType.substr(boundaryPos + 9);

    // Find filename in body
    size_t filenamePos = body.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return "";
    }

    filenamePos += 10; // Skip "filename=\""
    size_t filenameEnd = body.find("\"", filenamePos);
    if (filenameEnd == std::string::npos) {
        return "";
    }

    return body.substr(filenamePos, filenameEnd - filenamePos);
}

std::vector<char> UploadHandler::extractFileData(const std::string& contentType, const std::string& body) {
    // Extract boundary from content type
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return {};
    }

    std::string boundary = "--" + contentType.substr(boundaryPos + 9);

    // Find file data start
    size_t fileDataStart = body.find("\r\n\r\n");
    if (fileDataStart == std::string::npos) {
        fileDataStart = body.find("\n\n");
        if (fileDataStart == std::string::npos) {
            return {};
        }
        fileDataStart += 2;
    } else {
        fileDataStart += 4;
    }

    // Find file data end
    size_t fileDataEnd = body.find(boundary, fileDataStart);
    if (fileDataEnd == std::string::npos) {
        fileDataEnd = body.length();
    }

    // Extract file data
    std::vector<char> fileData(body.begin() + fileDataStart, body.begin() + fileDataEnd);
    return fileData;
}

std::string UploadHandler::handle(const std::string& body, const std::string& contentType) {
    // Extract filename
    std::string filename = extractFilename(contentType, body);
    if (filename.empty()) {
        return "400 Bad Request: No filename found";
    }

    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type";
    }

    // Extract file data
    std::vector<char> fileData = extractFileData(contentType, body);
    if (fileData.empty()) {
        return "400 Bad Request: No file data found";
    }

    // Generate timestamp for filename
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    std::string timestamp = oss.str();

    // Create save path
    std::string savePath = "Data/" + timestamp + "_" + filename;

    // Save file
    std::ofstream outFile(savePath, std::ios::binary);
    if (!outFile) {
        return "500 Internal Server Error: Could not save file";
    }

    outFile.write(fileData.data(), fileData.size());
    outFile.close();

    return "200 OK: File uploaded successfully";
}
