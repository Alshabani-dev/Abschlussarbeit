#include "UploadHandler.h"
#include <fstream>
#include <vector>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <cctype>

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    // Convert to lowercase for case-insensitive comparison
    std::string lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
        [](unsigned char c) { return std::tolower(c); });

    return lowerFilename.size() >= 4 &&
           (lowerFilename.compare(lowerFilename.size() - 4, 4, ".jpg") == 0 ||
            lowerFilename.compare(lowerFilename.size() - 5, 5, ".jpeg") == 0 ||
            lowerFilename.compare(lowerFilename.size() - 4, 4, ".png") == 0 ||
            lowerFilename.compare(lowerFilename.size() - 4, 4, ".gif") == 0 ||
            lowerFilename.compare(lowerFilename.size() - 4, 4, ".bmp") == 0);
}

std::string UploadHandler::handle(const std::string& body, const std::string& contentType) {
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

    // Find filename in the body
    size_t filenamePos = body.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return "400 Bad Request: No filename found";
    }

    size_t filenameStart = filenamePos + 10;
    size_t filenameEnd = body.find("\"", filenameStart);
    if (filenameEnd == std::string::npos) {
        return "400 Bad Request: Invalid filename format";
    }

    std::string filename = body.substr(filenameStart, filenameEnd - filenameStart);

    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type";
    }

    // Find file data start
    size_t fileDataStart = body.find("\r\n\r\n", filenameEnd);
    if (fileDataStart == std::string::npos) {
        fileDataStart = body.find("\n\n", filenameEnd);
        if (fileDataStart == std::string::npos) {
            return "400 Bad Request: No file data found";
        }
        fileDataStart += 2;
    } else {
        fileDataStart += 4;
    }

    // Find file data end (boundary)
    size_t fileDataEnd = body.find(boundary, fileDataStart);
    if (fileDataEnd == std::string::npos) {
        fileDataEnd = body.length();
    }

    // Extract file data using vector<char> for binary safety
    std::vector<char> fileData(body.begin() + fileDataStart, body.begin() + fileDataEnd);

    // Generate unique filename with timestamp
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", timeinfo);

    std::string uniqueFilename = "Data/" + std::string(timestamp) + "_" + filename;

    // Save file
    std::ofstream outFile(uniqueFilename, std::ios::binary);
    if (!outFile) {
        return "500 Internal Server Error: Could not save file";
    }

    outFile.write(&fileData[0], fileData.size());
    outFile.close();

    return "200 OK: File uploaded successfully";
}
