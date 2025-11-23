#include "UploadHandler.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cstring>

UploadHandler::UploadHandler() {}

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Extract boundary from Content-Type
    std::string boundaryPrefix = "boundary=";
    size_t boundaryPos = contentType.find(boundaryPrefix);
    if (boundaryPos == std::string::npos) {
        return "400 Bad Request: No boundary found";
    }
    
    std::string boundary = "--" + contentType.substr(boundaryPos + boundaryPrefix.length());
    
    // Extract filename
    std::string filename = extractFilename(body, boundary);
    if (filename.empty()) {
        return "400 Bad Request: No filename found";
    }
    
    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type";
    }
    
    // Extract file data
    std::vector<char> fileData = extractFileData(body, boundary);
    if (fileData.empty()) {
        return "400 Bad Request: No file data";
    }
    
    // Save file
    std::string savedPath = saveFile(filename, fileData);
    if (savedPath.empty()) {
        return "500 Internal Server Error: Could not save file";
    }
    
    return "200 OK";
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // C++17 compatible extension checking
    auto hasExtension = [&lower](const std::string& ext) {
        if (lower.length() < ext.length()) return false;
        return lower.compare(lower.length() - ext.length(), ext.length(), ext) == 0;
    };
    
    return hasExtension(".jpg") || 
           hasExtension(".jpeg") || 
           hasExtension(".png") || 
           hasExtension(".gif") || 
           hasExtension(".bmp");
}

std::string UploadHandler::extractFilename(const std::vector<char>& data, const std::string& boundary) {
    // Convert to string for searching (only the header part)
    std::string dataStr(data.begin(), data.begin() + std::min(data.size(), size_t(1024)));
    
    // Find filename in Content-Disposition header
    size_t filenamePos = dataStr.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return "";
    }
    
    filenamePos += 10; // Length of "filename=\""
    size_t filenameEnd = dataStr.find("\"", filenamePos);
    if (filenameEnd == std::string::npos) {
        return "";
    }
    
    return dataStr.substr(filenamePos, filenameEnd - filenamePos);
}

std::vector<char> UploadHandler::extractFileData(const std::vector<char>& data, const std::string& boundary) {
    // Convert to string for boundary searching
    std::string dataStr(data.begin(), data.end());
    
    // Find the filename position first
    size_t filename_end = dataStr.find("filename=\"");
    if (filename_end == std::string::npos) {
        return std::vector<char>();
    }
    
    filename_end = dataStr.find("\"", filename_end + 10);
    if (filename_end == std::string::npos) {
        return std::vector<char>();
    }
    
    // Find file data start (after headers) - flexible boundary detection
    size_t file_data_start = dataStr.find("\r\n\r\n", filename_end);
    if (file_data_start == std::string::npos) {
        file_data_start = dataStr.find("\n\n", filename_end);
        if (file_data_start == std::string::npos) {
            return std::vector<char>();
        }
        file_data_start += 2;
    } else {
        file_data_start += 4;
    }
    
    // Find file data end (before closing boundary) - handle missing boundary
    size_t file_data_end = dataStr.find(boundary, file_data_start);
    if (file_data_end == std::string::npos) {
        // No boundary found, use end of data
        file_data_end = data.size();
    }
    
    // Remove trailing newlines before boundary
    while (file_data_end > file_data_start && 
           (data[file_data_end - 1] == '\n' || data[file_data_end - 1] == '\r')) {
        file_data_end--;
    }
    
    // Extract binary data
    if (file_data_end <= file_data_start) {
        return std::vector<char>();
    }
    
    return std::vector<char>(data.begin() + file_data_start, data.begin() + file_data_end);
}

std::string UploadHandler::saveFile(const std::string& filename, const std::vector<char>& data) {
    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    
    // Create filename with timestamp
    std::ostringstream oss;
    oss << "Data/" << timestamp << "_" << filename;
    std::string filepath = oss.str();
    
    // Write binary data to file
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    file.write(data.data(), data.size());
    file.close();
    
    return filepath;
}
