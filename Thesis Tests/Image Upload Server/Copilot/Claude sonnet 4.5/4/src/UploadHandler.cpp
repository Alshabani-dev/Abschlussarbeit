#include "UploadHandler.h"
#include <fstream>
#include <algorithm>
#include <chrono>
#include <sstream>

UploadHandler::UploadHandler(const std::string& uploadDir) : uploadDir_(uploadDir) {}

bool UploadHandler::isValidImageExtension(const std::string& filename) const {
    // Find file extension
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

std::string UploadHandler::extractFilename(const std::string& data) const {
    // Look for filename in Content-Disposition header
    size_t filenamePos = data.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return "";
    }
    
    size_t nameStart = filenamePos + 10; // Length of "filename=\""
    size_t nameEnd = data.find("\"", nameStart);
    if (nameEnd == std::string::npos) {
        return "";
    }
    
    return data.substr(nameStart, nameEnd - nameStart);
}

std::string UploadHandler::generateUniqueFilename(const std::string& originalName) const {
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::ostringstream oss;
    oss << timestamp << "_" << originalName;
    return oss.str();
}

bool UploadHandler::saveFile(const std::string& filename, const std::vector<char>& data) {
    std::string filepath = uploadDir_ + "/" + filename;
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(data.data(), data.size());
    file.close();
    
    return true;
}

HttpResponse UploadHandler::handle(const std::vector<char>& body, const std::string& contentType) {
    // Extract boundary from Content-Type header
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return HttpResponse::badRequest("No boundary found in Content-Type");
    }
    
    std::string boundary = "--" + contentType.substr(boundaryPos + 9);
    
    // Convert body to string for parsing (we'll extract binary data separately)
    std::string bodyStr(body.begin(), body.end());
    
    // Find the first boundary
    size_t firstBoundary = bodyStr.find(boundary);
    if (firstBoundary == std::string::npos) {
        return HttpResponse::badRequest("Invalid multipart data");
    }
    
    // Look for Content-Disposition header with name="file"
    size_t filePartStart = bodyStr.find("name=\"file\"", firstBoundary);
    if (filePartStart == std::string::npos) {
        return HttpResponse::badRequest("No 'file' field found in upload");
    }
    
    // Extract filename
    std::string filename = extractFilename(bodyStr.substr(filePartStart));
    if (filename.empty()) {
        return HttpResponse::badRequest("No filename provided");
    }
    
    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return HttpResponse::badRequest("Invalid file type. Only JPG, JPEG, PNG, GIF, and BMP are allowed.");
    }
    
    // Find where the file data starts (after headers)
    // Try \r\n\r\n first, then \n\n
    size_t fileDataStart = bodyStr.find("\r\n\r\n", filePartStart);
    if (fileDataStart == std::string::npos) {
        fileDataStart = bodyStr.find("\n\n", filePartStart);
        if (fileDataStart == std::string::npos) {
            return HttpResponse::badRequest("No file data found");
        }
        fileDataStart += 2;
    } else {
        fileDataStart += 4;
    }
    
    // Find where the file data ends (next boundary or end of data)
    size_t fileDataEnd = bodyStr.find(boundary, fileDataStart);
    if (fileDataEnd == std::string::npos) {
        // No closing boundary, use end of data
        fileDataEnd = body.size();
    }
    
    // Extract file data (binary-safe)
    std::vector<char> fileData(body.begin() + fileDataStart, 
                               body.begin() + fileDataEnd);
    
    // Remove trailing newlines if present (before boundary)
    while (!fileData.empty() && (fileData.back() == '\r' || fileData.back() == '\n')) {
        fileData.pop_back();
    }
    
    // Generate unique filename
    std::string uniqueFilename = generateUniqueFilename(filename);
    
    // Save file
    if (!saveFile(uniqueFilename, fileData)) {
        return HttpResponse::internalError("Failed to save file");
    }
    
    // Return success response
    HttpResponse response;
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", "text/plain");
    response.setBody("File uploaded successfully: " + uniqueFilename);
    
    return response;
}
