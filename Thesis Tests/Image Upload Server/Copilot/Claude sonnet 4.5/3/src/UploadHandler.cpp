#include "UploadHandler.h"
#include <fstream>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <sys/stat.h>

UploadHandler::UploadHandler() : dataDir_("Data/") {}

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Extract boundary from Content-Type
    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return "400 Bad Request: No boundary found";
    }
    
    // Add -- prefix to boundary for parsing
    std::string fullBoundary = "--" + boundary;
    
    // Convert body to string for easier parsing (we'll extract binary data separately)
    std::string data(body.begin(), body.end());
    
    // Find the start of the file field
    size_t fileFieldPos = data.find("name=\"file\"");
    if (fileFieldPos == std::string::npos) {
        return "400 Bad Request: Missing 'file' field";
    }
    
    // Extract filename
    size_t filenamePos = data.find("filename=\"", fileFieldPos);
    if (filenamePos == std::string::npos) {
        return "400 Bad Request: No filename found";
    }
    
    filenamePos += 10; // Skip 'filename="'
    size_t filenameEnd = data.find("\"", filenamePos);
    if (filenameEnd == std::string::npos) {
        return "400 Bad Request: Invalid filename format";
    }
    
    std::string filename = data.substr(filenamePos, filenameEnd - filenamePos);
    
    // Validate extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type. Only JPG, PNG, GIF, BMP allowed";
    }
    
    // Find file data start - support both \r\n\r\n and \n\n
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
    
    // Find file data end (next boundary or end of data)
    size_t file_data_end = data.find(fullBoundary, file_data_start);
    if (file_data_end == std::string::npos) {
        file_data_end = data.length();
    }
    
    // Extract file content (binary-safe)
    std::vector<char> fileContent(body.begin() + file_data_start, 
                                   body.begin() + file_data_end);
    
    // Remove trailing newlines that might be part of multipart formatting
    while (!fileContent.empty() && 
           (fileContent.back() == '\r' || fileContent.back() == '\n')) {
        fileContent.pop_back();
    }
    
    // Generate timestamped filename
    std::string savedFilename = generateTimestampFilename(filename);
    
    // Save file
    if (!saveFile(savedFilename, fileContent)) {
        return "500 Internal Server Error: Could not save file";
    }
    
    return "200 OK: File uploaded successfully";
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    // Find extension
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    
    std::string ext = filename.substr(dotPos + 1);
    
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    return (ext == "jpg" || ext == "jpeg" || ext == "png" || 
            ext == "gif" || ext == "bmp");
}

std::string UploadHandler::extractBoundary(const std::string& contentType) {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "";
    }
    
    boundaryPos += 9; // Skip "boundary="
    
    // Boundary might be quoted or not
    std::string boundary;
    if (boundaryPos < contentType.length() && contentType[boundaryPos] == '"') {
        boundaryPos++; // Skip opening quote
        size_t endQuote = contentType.find('"', boundaryPos);
        if (endQuote != std::string::npos) {
            boundary = contentType.substr(boundaryPos, endQuote - boundaryPos);
        }
    } else {
        // Not quoted, read until semicolon or end
        size_t endPos = contentType.find(';', boundaryPos);
        if (endPos == std::string::npos) {
            boundary = contentType.substr(boundaryPos);
        } else {
            boundary = contentType.substr(boundaryPos, endPos - boundaryPos);
        }
    }
    
    // Trim whitespace
    size_t start = boundary.find_first_not_of(" \t\r\n");
    size_t end = boundary.find_last_not_of(" \t\r\n");
    if (start != std::string::npos && end != std::string::npos) {
        boundary = boundary.substr(start, end - start + 1);
    }
    
    return boundary;
}

bool UploadHandler::saveFile(const std::string& filename, const std::vector<char>& content) {
    std::string fullPath = dataDir_ + filename;
    
    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(content.data(), content.size());
    file.close();
    
    return file.good();
}

std::string UploadHandler::generateTimestampFilename(const std::string& originalName) {
    // Get current timestamp in milliseconds
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    std::ostringstream oss;
    oss << millis << "_" << originalName;
    return oss.str();
}
