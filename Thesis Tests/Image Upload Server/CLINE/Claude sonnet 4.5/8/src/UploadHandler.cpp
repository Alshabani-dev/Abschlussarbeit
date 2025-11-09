#include "UploadHandler.h"
#include <fstream>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>

UploadHandler::UploadHandler() {
}

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Check if contentType contains multipart/form-data
    if (contentType.find("multipart/form-data") == std::string::npos) {
        return "400 Bad Request: Not multipart/form-data";
    }
    
    // Extract boundary
    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return "400 Bad Request: No boundary found";
    }
    
    // Convert body to string for parsing (temporary for finding boundaries)
    std::string bodyStr(body.begin(), body.end());
    
    // Find the file field
    size_t fileFieldStart = bodyStr.find("Content-Disposition:");
    if (fileFieldStart == std::string::npos) {
        return "400 Bad Request: No Content-Disposition found";
    }
    
    // Check if it's a file field (should contain filename=)
    size_t filenamePos = bodyStr.find("filename=", fileFieldStart);
    if (filenamePos == std::string::npos) {
        return "400 Bad Request: No filename found (use 'file' field name)";
    }
    
    // Extract filename
    std::string filename = extractFilename(bodyStr.substr(fileFieldStart));
    if (filename.empty()) {
        return "400 Bad Request: Empty filename";
    }
    
    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type (only JPG, PNG, GIF, BMP allowed)";
    }
    
    // Find where file data starts (after headers within this part)
    size_t filename_end = filenamePos + filename.length() + 20; // rough estimate
    size_t file_data_start = bodyStr.find("\r\n\r\n", filename_end);
    if (file_data_start == std::string::npos) {
        file_data_start = bodyStr.find("\n\n", filename_end);
        if (file_data_start == std::string::npos) {
            return "400 Bad Request: No file data found";
        }
        file_data_start += 2;
    } else {
        file_data_start += 4;
    }
    
    // Find where file data ends (before the closing boundary)
    // Look for boundary with leading newline
    std::string closingBoundary = "--" + boundary;
    size_t file_data_end = bodyStr.find(closingBoundary, file_data_start);
    if (file_data_end == std::string::npos) {
        // If no boundary found, use end of data
        file_data_end = body.size();
    } else {
        // Move back to exclude the newline before boundary
        if (file_data_end > 0 && bodyStr[file_data_end - 1] == '\n') {
            file_data_end--;
            if (file_data_end > 0 && bodyStr[file_data_end - 1] == '\r') {
                file_data_end--;
            }
        }
    }
    
    // Extract file data (binary-safe)
    if (file_data_end <= file_data_start) {
        return "400 Bad Request: Empty file data";
    }
    
    std::vector<char> fileData(body.begin() + file_data_start, body.begin() + file_data_end);
    
    // Generate output filename with timestamp
    std::string timestamp = generateTimestamp();
    std::string outputFilename = "Data/" + timestamp + "_" + filename;
    
    // Save file
    if (!saveFile(outputFilename, fileData)) {
        return "500 Internal Server Error: Failed to save file";
    }
    
    return "200 OK: File uploaded successfully";
}

bool UploadHandler::isValidImageExtension(const std::string& filename) const {
    std::string ext = getFileExtension(filename);
    return (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp");
}

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
    
    // Trim any trailing whitespace
    size_t end = boundary.find_first_of(" \r\n\t");
    if (end != std::string::npos) {
        boundary = boundary.substr(0, end);
    }
    
    return boundary;
}

std::string UploadHandler::extractFilename(const std::string& data) const {
    size_t filenamePos = data.find("filename=");
    if (filenamePos == std::string::npos) {
        return "";
    }
    
    size_t start = filenamePos + 9;
    
    // Check if filename is quoted
    if (start < data.size() && data[start] == '"') {
        start++;
        size_t end = data.find('"', start);
        if (end != std::string::npos) {
            return data.substr(start, end - start);
        }
    } else {
        // Not quoted, find end by space or newline
        size_t end = data.find_first_of(" \r\n;", start);
        if (end != std::string::npos) {
            return data.substr(start, end - start);
        }
    }
    
    return "";
}

std::string UploadHandler::getFileExtension(const std::string& filename) const {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos && dotPos < filename.length() - 1) {
        std::string ext = filename.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    return "";
}

std::string UploadHandler::generateTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << timestamp;
    return oss.str();
}

bool UploadHandler::saveFile(const std::string& filepath, const std::vector<char>& data) {
    // Create Data directory if it doesn't exist
    mkdir("Data", 0755);
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(data.data(), data.size());
    file.close();
    
    return true;
}
