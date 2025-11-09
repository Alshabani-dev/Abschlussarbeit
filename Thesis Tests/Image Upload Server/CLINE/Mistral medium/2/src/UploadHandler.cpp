#include "UploadHandler.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>
#include <algorithm>

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    static const std::vector<std::string> validExtensions = {".jpg", ".jpeg", ".png", ".gif", ".bmp"};
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string extension = filename.substr(dotPos);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        for (const auto& validExt : validExtensions) {
            if (extension == validExt) {
                return true;
            }
        }
    }
    return false;
}

std::string UploadHandler::handle(const std::string& body, const std::string& contentType) {
    // Extract boundary from content type
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "400 Bad Request: No boundary found";
    }
    std::string boundary = "--" + contentType.substr(boundaryPos + 9);

    // Find filename in body
    size_t filenameStart = body.find("filename=\"");
    if (filenameStart == std::string::npos) {
        return "400 Bad Request: No filename found";
    }
    filenameStart += 10;
    size_t filenameEnd = body.find("\"", filenameStart);
    if (filenameEnd == std::string::npos) {
        return "400 Bad Request: Invalid filename format";
    }
    std::string filename = body.substr(filenameStart, filenameEnd - filenameStart);

    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type";
    }

    // Find file data start and end
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

    size_t fileDataEnd = body.find(boundary, fileDataStart);
    if (fileDataEnd == std::string::npos) {
        fileDataEnd = body.length();
    }

    // Save file
    std::time_t now = std::time(nullptr);
    std::string timestamp = std::to_string(now);
    std::string savePath = "Data/" + timestamp + "_" + filename;

    std::ofstream outFile(savePath, std::ios::binary);
    if (!outFile) {
        return "500 Internal Server Error: Failed to save file";
    }

    outFile.write(body.data() + fileDataStart, fileDataEnd - fileDataStart);
    outFile.close();

    return "200 OK: File uploaded successfully";
}
