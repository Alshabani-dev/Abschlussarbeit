#include "UploadHandler.h"
#include <fstream>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iostream>

UploadHandler::UploadHandler(const std::string& uploadDir) : uploadDir_(uploadDir) {}

std::string UploadHandler::extractBoundary(const std::string& contentType) const {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "";
    }
    
    std::string boundary = contentType.substr(boundaryPos + 9);
    // Remove quotes if present
    if (!boundary.empty() && boundary[0] == '"') {
        boundary = boundary.substr(1);
    }
    if (!boundary.empty() && boundary.back() == '"') {
        boundary.pop_back();
    }
    
    return "--" + boundary;
}

std::string UploadHandler::extractFilename(const std::string& data, size_t start, size_t end) const {
    std::string section = data.substr(start, end - start);
    
    size_t filenamePos = section.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return "";
    }
    
    size_t filenameStart = filenamePos + 10;
    size_t filenameEnd = section.find("\"", filenameStart);
    
    if (filenameEnd == std::string::npos) {
        return "";
    }
    
    return section.substr(filenameStart, filenameEnd - filenameStart);
}

bool UploadHandler::isValidImageExtension(const std::string& filename) const {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    
    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return (ext == "jpg" || ext == "jpeg" || ext == "png" || 
            ext == "gif" || ext == "bmp");
}

std::string UploadHandler::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    return std::to_string(timestamp);
}

bool UploadHandler::saveFile(const std::string& filename, const std::vector<char>& data) {
    std::string fullPath = uploadDir_ + "/" + filename;
    
    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(data.data(), data.size());
    file.close();
    
    return true;
}

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Extract boundary
    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return "400 Bad Request: No boundary found";
    }
    
    // Convert body to string for parsing (we'll extract binary data separately)
    std::string data(body.begin(), body.end());
    
    // Find the first boundary
    size_t boundaryPos = data.find(boundary);
    if (boundaryPos == std::string::npos) {
        return "400 Bad Request: Invalid multipart data";
    }
    
    // Find Content-Disposition header
    size_t dispositionPos = data.find("Content-Disposition:", boundaryPos);
    if (dispositionPos == std::string::npos) {
        return "400 Bad Request: No Content-Disposition header";
    }
    
    // Check if this is the "file" field
    size_t namePos = data.find("name=\"file\"", dispositionPos);
    if (namePos == std::string::npos || namePos > data.find("\r\n\r\n", dispositionPos)) {
        // Check for just newlines
        size_t altEnd = data.find("\n\n", dispositionPos);
        if (namePos == std::string::npos || namePos > altEnd) {
            return "400 Bad Request: Missing 'file' field";
        }
    }
    
    // Extract filename
    size_t filenamePos = data.find("filename=\"", dispositionPos);
    if (filenamePos == std::string::npos) {
        return "400 Bad Request: No filename found";
    }
    
    size_t filenameStart = filenamePos + 10;
    size_t filenameEnd = data.find("\"", filenameStart);
    if (filenameEnd == std::string::npos) {
        return "400 Bad Request: Invalid filename";
    }
    
    std::string originalFilename = data.substr(filenameStart, filenameEnd - filenameStart);
    
    // Validate file extension
    if (!isValidImageExtension(originalFilename)) {
        return "400 Bad Request: Invalid file type";
    }
    
    // Find where file data starts (after headers)
    // Using flexible boundary detection as per README
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
    
    // Find where file data ends (at the next boundary)
    size_t file_data_end = data.find(boundary, file_data_start);
    if (file_data_end == std::string::npos) {
        // Fallback to data end if boundary is missing
        file_data_end = data.length();
    }
    
    // Extract file data (binary-safe using vector)
    std::vector<char> fileData(body.begin() + file_data_start, 
                                body.begin() + file_data_end);
    
    // Remove trailing newlines from file data if present
    while (!fileData.empty() && (fileData.back() == '\r' || fileData.back() == '\n')) {
        fileData.pop_back();
    }
    
    // Generate filename with timestamp
    std::string newFilename = getCurrentTimestamp() + "_" + originalFilename;
    
    // Save file
    if (!saveFile(newFilename, fileData)) {
        return "500 Internal Server Error: Failed to save file";
    }
    
    return "200 OK";
}
