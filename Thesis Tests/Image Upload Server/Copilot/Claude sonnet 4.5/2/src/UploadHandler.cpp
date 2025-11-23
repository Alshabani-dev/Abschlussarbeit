#include "UploadHandler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Extract boundary
    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return "400 Bad Request: No boundary found";
    }
    
    // Convert body to string for parsing (we'll extract binary data separately)
    std::string data(body.begin(), body.end());
    
    // Find the "file" field (not "wrong" or other fields)
    size_t file_field_pos = data.find("name=\"file\"");
    if (file_field_pos == std::string::npos) {
        return "400 Bad Request: No file field found";
    }
    
    // Find filename
    size_t filename_pos = data.find("filename=\"", file_field_pos);
    if (filename_pos == std::string::npos) {
        return "400 Bad Request: No filename found";
    }
    filename_pos += 10; // Skip 'filename="'
    
    size_t filename_end = data.find("\"", filename_pos);
    if (filename_end == std::string::npos) {
        return "400 Bad Request: Invalid filename format";
    }
    
    std::string filename = data.substr(filename_pos, filename_end - filename_pos);
    
    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type";
    }
    
    // Find the start of file data (after headers)
    // Try \r\n\r\n first, then \n\n
    size_t file_data_start = data.find("\r\n\r\n", filename_end);
    if (file_data_start == std::string::npos) {
        file_data_start = data.find("\n\n", filename_end);
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
        // If boundary not found, use end of data
        file_data_end = data.length();
    }
    
    // Extract file data (binary-safe using vector)
    std::vector<char> file_data(body.begin() + file_data_start, body.begin() + file_data_end);
    
    // Remove trailing newlines that might be before the boundary
    while (!file_data.empty() && (file_data.back() == '\r' || file_data.back() == '\n')) {
        file_data.pop_back();
    }
    
    // Save file
    if (!saveFile(filename, file_data)) {
        return "500 Internal Server Error: Failed to save file";
    }
    
    return "200 OK";
}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    // Find file extension
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return false;
    }
    
    std::string ext = filename.substr(dot_pos + 1);
    
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Check valid extensions
    return (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp");
}

std::string UploadHandler::extractBoundary(const std::string& contentType) {
    size_t boundary_pos = contentType.find("boundary=");
    if (boundary_pos == std::string::npos) {
        return "";
    }
    
    boundary_pos += 9; // Skip "boundary="
    std::string boundary = contentType.substr(boundary_pos);
    
    // Remove any trailing whitespace or semicolons
    size_t end_pos = boundary.find_first_of("; \t\r\n");
    if (end_pos != std::string::npos) {
        boundary = boundary.substr(0, end_pos);
    }
    
    // Add dashes prefix
    return "--" + boundary;
}

bool UploadHandler::saveFile(const std::string& filename, const std::vector<char>& data) {
    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    
    // Create filename with timestamp
    std::ostringstream oss;
    oss << "Data/" << timestamp << "_" << filename;
    std::string filepath = oss.str();
    
    // Write file (binary mode)
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(data.data(), data.size());
    file.close();
    
    return true;
}
