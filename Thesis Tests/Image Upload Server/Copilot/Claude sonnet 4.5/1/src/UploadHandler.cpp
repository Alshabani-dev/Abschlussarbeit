#include "UploadHandler.h"
#include <fstream>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cstring>

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Extract boundary from Content-Type header
    std::string boundaryPrefix = "boundary=";
    size_t boundaryPos = contentType.find(boundaryPrefix);
    if (boundaryPos == std::string::npos) {
        return "400 Bad Request: No boundary in Content-Type";
    }
    
    std::string boundary = "--" + contentType.substr(boundaryPos + boundaryPrefix.length());
    
    // Convert body to string for parsing (we'll extract binary data separately)
    std::string data(body.begin(), body.end());
    
    // Find the file field
    size_t fileFieldPos = data.find("Content-Disposition:");
    if (fileFieldPos == std::string::npos) {
        return "400 Bad Request: No Content-Disposition found";
    }
    
    // Extract Content-Disposition line
    size_t dispEnd = data.find("\n", fileFieldPos);
    if (dispEnd == std::string::npos) {
        return "400 Bad Request: Invalid Content-Disposition";
    }
    
    std::string contentDisposition = data.substr(fileFieldPos, dispEnd - fileFieldPos);
    
    // Check if this is the "file" field
    if (contentDisposition.find("name=\"file\"") == std::string::npos) {
        return "400 Bad Request: Expected field name 'file'";
    }
    
    // Extract filename
    std::string filename = extractFilename(contentDisposition);
    if (filename.empty()) {
        return "400 Bad Request: No filename provided";
    }
    
    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type. Only JPG, PNG, GIF, and BMP are allowed.";
    }
    
    // Find where the file data starts (after all part headers)
    // Start searching from the Content-Disposition line
    size_t search_pos = fileFieldPos;
    
    // Find file data start - support both \r\n\r\n and \n\n
    size_t file_data_start = data.find("\r\n\r\n", search_pos);
    if (file_data_start == std::string::npos) {
        file_data_start = data.find("\n\n", search_pos);
        if (file_data_start == std::string::npos) {
            return "400 Bad Request: No file data found";
        }
        file_data_start += 2;
    } else {
        file_data_start += 4;
    }
    
    // Find file data end (boundary)
    size_t file_data_end = data.find(boundary, file_data_start);
    if (file_data_end == std::string::npos) {
        // Fallback to end of data if boundary not found
        file_data_end = data.length();
    }
    
    // Extract file data (binary-safe using vector)
    std::vector<char> fileData;
    if (file_data_start < body.size() && file_data_end <= body.size()) {
        fileData.assign(body.begin() + file_data_start, body.begin() + file_data_end);
    } else {
        return "400 Bad Request: Invalid file data range";
    }
    
    // Remove trailing newlines from file data
    while (!fileData.empty() && (fileData.back() == '\n' || fileData.back() == '\r')) {
        fileData.pop_back();
    }
    
    if (fileData.empty()) {
        return "400 Bad Request: Empty file data";
    }
    
    // Generate unique filename with timestamp
    std::string timestamp = generateTimestamp();
    std::string savedFilename = timestamp + "_" + filename;
    std::string filepath = "Data/" + savedFilename;
    
    // Save file
    if (!saveFile(filepath, fileData)) {
        return "500 Internal Server Error: Failed to save file";
    }
    
    return "200 OK: File uploaded successfully as " + savedFilename;
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    std::string ext = getFileExtension(filename);
    return (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp");
}

std::string UploadHandler::extractFilename(const std::string& contentDisposition) {
    size_t filenamePos = contentDisposition.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return "";
    }
    
    filenamePos += 10; // Length of "filename=\""
    size_t filenameEnd = contentDisposition.find("\"", filenamePos);
    if (filenameEnd == std::string::npos) {
        return "";
    }
    
    return contentDisposition.substr(filenamePos, filenameEnd - filenamePos);
}

std::string UploadHandler::getFileExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    
    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool UploadHandler::saveFile(const std::string& filepath, const std::vector<char>& data) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(data.data(), data.size());
    file.close();
    
    return file.good();
}

std::string UploadHandler::generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    return std::to_string(timestamp);
}
