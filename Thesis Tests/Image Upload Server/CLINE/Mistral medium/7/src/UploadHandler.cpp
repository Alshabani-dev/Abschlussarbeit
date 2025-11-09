#include "UploadHandler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <vector>

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp";
}

std::string UploadHandler::extractFilename(const std::string& contentType, const std::string& body) {
    // Extract boundary from content type
    std::string boundary = "--";
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos != std::string::npos) {
        boundary += contentType.substr(boundaryPos + 9);
    }

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

std::string UploadHandler::extractFileData(const std::string& boundary, const std::string& body) {
    size_t fileDataStart = body.find("\r\n\r\n");
    if (fileDataStart == std::string::npos) {
        fileDataStart = body.find("\n\n");
        if (fileDataStart == std::string::npos) {
            return "";
        }
        fileDataStart += 2;
    } else {
        fileDataStart += 4;
    }

    size_t fileDataEnd = body.find(boundary, fileDataStart);
    if (fileDataEnd == std::string::npos) {
        fileDataEnd = body.length();
    }

    return body.substr(fileDataStart, fileDataEnd - fileDataStart);
}

std::string UploadHandler::generateUniqueFilename(const std::string& originalFilename) {
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", timeinfo);

    std::string filename = originalFilename;
    size_t dotPos = filename.find_last_of(".");
    std::string extension = (dotPos != std::string::npos) ? filename.substr(dotPos) : "";

    return std::string(timestamp) + "_" + filename.substr(0, dotPos) + extension;
}

std::string UploadHandler::handle(const std::string& body, const std::string& contentType) {
    std::string filename = extractFilename(contentType, body);
    if (filename.empty() || !isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type or no filename provided";
    }

    std::string boundary = "--";
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos != std::string::npos) {
        boundary += contentType.substr(boundaryPos + 9);
    }

    std::string fileData = extractFileData(boundary, body);
    if (fileData.empty()) {
        return "400 Bad Request: No file data found";
    }

    std::string uniqueFilename = generateUniqueFilename(filename);
    std::string filePath = "Data/" + uniqueFilename;

    // Create Data directory if it doesn't exist
    if (!std::filesystem::exists("Data")) {
        std::filesystem::create_directory("Data");
    }

    // Save file
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile) {
        return "500 Internal Server Error: Could not save file";
    }

    outFile.write(fileData.data(), fileData.size());
    outFile.close();

    return "200 OK: File uploaded successfully as " + uniqueFilename;
}
