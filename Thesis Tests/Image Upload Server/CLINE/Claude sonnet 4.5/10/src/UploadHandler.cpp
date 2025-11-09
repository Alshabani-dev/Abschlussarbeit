#include "UploadHandler.h"
#include <fstream>
#include <algorithm>
#include <chrono>
#include <sstream>

UploadHandler::UploadHandler() {}

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Extract boundary from Content-Type
    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return "400 Bad Request: No boundary found";
    }
    
    // Convert body to string for parsing (we'll extract binary data separately)
    std::string data(body.begin(), body.end());
    
    // Find "filename=" in the multipart data
    size_t filenamePos = data.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return "400 Bad Request: No file uploaded";
    }
    
    // Extract filename
    filenamePos += 10; // Length of "filename=\""
    size_t filenameEnd = data.find("\"", filenamePos);
    if (filenameEnd == std::string::npos) {
        return "400 Bad Request: Invalid filename";
    }
    
    std::string filename = data.substr(filenamePos, filenameEnd - filenamePos);
    if (filename.empty()) {
        return "400 Bad Request: Empty filename";
    }
    
    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type. Only JPG, PNG, GIF, and BMP are allowed";
    }
    
    // Find the start of file data - try \r\n\r\n first, then \n\n
    size_t file_data_start = data.find("\r\n\r\n", filenameEnd);
    if (file_data_start == std::string::npos) {
        file_data_start = data.find("\n\n", filenameEnd);
        if (file_data_start == std::string::npos) {
            return "400 Bad Request: No file data found";
        }
        file_data_start += 2;
    } else {
        file_data_start += 4;
    }
    
    // Find the end of file data (boundary marker)
    size_t file_data_end = data.find(boundary, file_data_start);
    if (file_data_end == std::string::npos) {
        file_data_end = body.size();
    }
    
    // Extract binary file data
    std::vector<char> fileData(body.begin() + file_data_start, body.begin() + file_data_end);
    
    // Remove trailing newlines from file data
    while (!fileData.empty() && (fileData.back() == '\r' || fileData.back() == '\n')) {
        fileData.pop_back();
    }
    
    // Generate unique filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string uniqueFilename = "Data/" + std::to_string(timestamp) + "_" + filename;
    
    // Save file
    if (!saveFile(uniqueFilename, fileData)) {
        return "500 Internal Server Error: Failed to save file";
    }
    
    return "200 OK";
}

std::string UploadHandler::extractBoundary(const std::string& contentType) {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "";
    }
    
    std::string boundary = contentType.substr(boundaryPos + 9);
    // Add -- prefix to boundary
    return "--" + boundary;
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    std::string ext = getExtension(filename);
    return (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp");
}

std::string UploadHandler::getExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    return toLower(filename.substr(dotPos + 1));
}

std::string UploadHandler::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool UploadHandler::saveFile(const std::string& filename, const std::vector<char>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(data.data(), data.size());
    return file.good();
}
