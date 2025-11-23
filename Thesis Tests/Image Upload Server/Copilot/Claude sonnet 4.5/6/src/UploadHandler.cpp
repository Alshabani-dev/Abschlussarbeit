#include "UploadHandler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>

UploadHandler::UploadHandler() {}

bool UploadHandler::isValidImageExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    
    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return (ext == "jpg" || ext == "jpeg" || ext == "png" || 
            ext == "gif" || ext == "bmp");
}

std::string UploadHandler::extractBoundary(const std::string& contentType) {
    size_t pos = contentType.find("boundary=");
    if (pos == std::string::npos) {
        return "";
    }
    
    std::string boundary = contentType.substr(pos + 9);
    // Remove any trailing whitespace or semicolons
    size_t end = boundary.find_first_of(" ;\r\n");
    if (end != std::string::npos) {
        boundary = boundary.substr(0, end);
    }
    
    return "--" + boundary;
}

std::string UploadHandler::saveFile(const std::string& filename, const std::vector<char>& data) {
    // Create timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream filenameStream;
    filenameStream << "Data/" << timestamp << "_" << filename;
    std::string filepath = filenameStream.str();
    
    // Save file in binary mode
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    file.write(data.data(), data.size());
    file.close();
    
    return filepath;
}

std::string UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Extract boundary
    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return "400 Bad Request: No boundary found";
    }
    
    // Convert body to string for parsing (we'll extract binary data separately)
    std::string data(body.begin(), body.end());
    
    // Find the file field
    size_t file_field_pos = data.find("Content-Disposition: form-data;");
    if (file_field_pos == std::string::npos) {
        return "400 Bad Request: No form-data found";
    }
    
    // Check if field name is "file"
    size_t name_pos = data.find("name=\"", file_field_pos);
    if (name_pos == std::string::npos) {
        return "400 Bad Request: No name field found";
    }
    
    name_pos += 6; // Skip 'name="'
    size_t name_end = data.find("\"", name_pos);
    if (name_end == std::string::npos) {
        return "400 Bad Request: Malformed name field";
    }
    
    std::string field_name = data.substr(name_pos, name_end - name_pos);
    if (field_name != "file") {
        return "400 Bad Request: Field name must be 'file'";
    }
    
    // Extract filename
    size_t filename_pos = data.find("filename=\"", name_end);
    if (filename_pos == std::string::npos) {
        return "400 Bad Request: No filename found";
    }
    
    filename_pos += 10; // Skip 'filename="'
    size_t filename_end = data.find("\"", filename_pos);
    if (filename_end == std::string::npos) {
        return "400 Bad Request: Malformed filename";
    }
    
    std::string filename = data.substr(filename_pos, filename_end - filename_pos);
    
    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type. Only JPG, JPEG, PNG, GIF, BMP allowed";
    }
    
    // Find file data start - support both \r\n\r\n and \n\n
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
    
    // Find file data end (boundary or end of data)
    size_t file_data_end = data.find(boundary, file_data_start);
    if (file_data_end == std::string::npos) {
        file_data_end = data.length();
    }
    
    // Extract binary file data from body vector
    std::vector<char> file_data(body.begin() + file_data_start, 
                                 body.begin() + file_data_end);
    
    // Remove trailing newlines if present
    while (!file_data.empty() && (file_data.back() == '\r' || file_data.back() == '\n')) {
        file_data.pop_back();
    }
    
    // Save file
    std::string savedPath = saveFile(filename, file_data);
    if (savedPath.empty()) {
        return "500 Internal Server Error: Could not save file";
    }
    
    return "200 OK";
}
