#include "UploadHandler.h"
#include <fstream>
#include <vector>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <iomanip>

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) return false;

    std::string ext = filename.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp";
}

std::string UploadHandler::handle(const std::string& body, const std::string& contentType) {
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

    // Find filename in the body
    size_t filenamePos = body.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return "400 Bad Request: No filename found";
    }

    filenamePos += 10; // Skip "filename=\""
    size_t filenameEnd = body.find("\"", filenamePos);
    if (filenameEnd == std::string::npos) {
        return "400 Bad Request: Invalid filename format";
    }

    std::string filename = body.substr(filenamePos, filenameEnd - filenamePos);

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

    // Extract file data
    std::string fileData = body.substr(fileDataStart, fileDataEnd - fileDataStart);

    // Generate timestamp for filename
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream timestamp;
    timestamp << std::put_time(&tm, "%Y%m%d_%H%M%S");

    // Create output filename
    std::string outputFilename = "Data/" + timestamp.str() + "_" + filename;

    // Save file
    std::ofstream outFile(outputFilename, std::ios::binary);
    if (!outFile) {
        return "500 Internal Server Error: Could not save file";
    }

    outFile.write(fileData.c_str(), fileData.size());
    outFile.close();

    return "200 OK: File uploaded successfully";
}
