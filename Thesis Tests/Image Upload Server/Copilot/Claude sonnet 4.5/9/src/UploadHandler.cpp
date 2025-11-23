#include "UploadHandler.h"
#include "FileHandler.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iostream>

UploadHandler::UploadHandler() {}

std::string UploadHandler::extractBoundary(const std::string& contentType) {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "";
    }
    
    std::string boundary = contentType.substr(boundaryPos + 9);
    // Remove any trailing whitespace or semicolons
    size_t end = boundary.find_first_of("; \t\r\n");
    if (end != std::string::npos) {
        boundary = boundary.substr(0, end);
    }
    
    return "--" + boundary;
}

std::string UploadHandler::extractFilename(const std::string& data, size_t start, size_t end) {
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

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    return lower.find(".jpg") != std::string::npos ||
           lower.find(".jpeg") != std::string::npos ||
           lower.find(".png") != std::string::npos ||
           lower.find(".gif") != std::string::npos ||
           lower.find(".bmp") != std::string::npos;
}

std::string UploadHandler::generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    return std::to_string(timestamp);
}

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    if (body.empty()) {
        return "400 Bad Request: Empty body";
    }
    
    // Extract boundary
    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return "400 Bad Request: No boundary found";
    }
    
    // Convert body to string for parsing (needed for string operations)
    std::string data(body.begin(), body.end());
    
    // Find the first boundary
    size_t firstBoundary = data.find(boundary);
    if (firstBoundary == std::string::npos) {
        return "400 Bad Request: No boundary in body";
    }
    
    // Move past the first boundary
    size_t partStart = firstBoundary + boundary.length();
    
    // Skip newlines after boundary
    while (partStart < data.length() && (data[partStart] == '\r' || data[partStart] == '\n')) {
        partStart++;
    }
    
    // Find next boundary
    size_t nextBoundary = data.find(boundary, partStart);
    if (nextBoundary == std::string::npos) {
        nextBoundary = data.length();
    }
    
    // Extract the part content
    std::string part = data.substr(partStart, nextBoundary - partStart);
    
    // Check if field name is "file"
    if (part.find("name=\"file\"") == std::string::npos) {
        return "400 Bad Request: Field name must be 'file'";
    }
    
    // Extract filename
    std::string filename = extractFilename(part, 0, part.length());
    if (filename.empty()) {
        return "400 Bad Request: No filename found";
    }
    
    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type. Only jpg, jpeg, png, gif, bmp allowed";
    }
    
    // Find the start of file data (after headers)
    size_t file_data_start = part.find("\r\n\r\n");
    if (file_data_start == std::string::npos) {
        file_data_start = part.find("\n\n");
        if (file_data_start == std::string::npos) {
            return "400 Bad Request: No file data found";
        }
        file_data_start += 2;
    } else {
        file_data_start += 4;
    }
    
    // Find the end of file data (before closing boundary or end of part)
    size_t file_data_end = part.length();
    
    // Trim trailing newlines before boundary
    while (file_data_end > file_data_start && 
           (part[file_data_end - 1] == '\r' || part[file_data_end - 1] == '\n')) {
        file_data_end--;
    }
    
    // Extract file data (binary-safe)
    size_t absoluteStart = partStart + file_data_start;
    size_t absoluteEnd = partStart + file_data_end;
    
    if (absoluteStart >= body.size() || absoluteEnd > body.size() || absoluteStart >= absoluteEnd) {
        return "400 Bad Request: Invalid file data range";
    }
    
    std::vector<char> fileData(body.begin() + absoluteStart, body.begin() + absoluteEnd);
    
    if (fileData.empty()) {
        return "400 Bad Request: Empty file data";
    }
    
    // Generate filename with timestamp
    std::string timestamp = generateTimestamp();
    std::string outputFilename = "Data/" + timestamp + "_" + filename;
    
    // Save file
    FileHandler fileHandler;
    if (!fileHandler.writeFile(outputFilename, fileData)) {
        return "500 Internal Server Error: Failed to save file";
    }
    
    return "200 OK: File uploaded successfully as " + outputFilename;
}
