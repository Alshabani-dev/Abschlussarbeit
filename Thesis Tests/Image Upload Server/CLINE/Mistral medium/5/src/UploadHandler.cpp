#include "UploadHandler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sys/stat.h>
#include <unistd.h>

UploadHandler::UploadHandler(const std::string& upload_dir) : upload_dir_(upload_dir) {
    // Create upload directory if it doesn't exist
    if (mkdir(upload_dir_.c_str(), 0755) != 0) {
        if (errno != EEXIST) {
            throw std::runtime_error("Failed to create upload directory: " + upload_dir_);
        }
    }
}

UploadHandler::~UploadHandler() {}

std::string UploadHandler::handleUpload(const std::vector<char>& body, const std::string& content_type) {
    try {
        std::string boundary = extractBoundary(content_type);
        if (boundary.empty()) {
            return "400 Bad Request: No boundary found in Content-Type";
        }

        return parseMultipartFormData(body, boundary);
    } catch (const std::exception& e) {
        return std::string("500 Internal Server Error: ") + e.what();
    }
}

bool UploadHandler::isValidImageExtension(const std::string& filename) const {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return false;
    }

    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return std::tolower(c); });

    return (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp");
}

std::string UploadHandler::generateFilename(const std::string& original_name) const {
    // Get current timestamp
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");

    // Extract extension
    size_t dot_pos = original_name.find_last_of('.');
    std::string ext = (dot_pos != std::string::npos) ? original_name.substr(dot_pos) : "";

    return oss.str() + "_" + original_name + ext;
}

std::string UploadHandler::extractBoundary(const std::string& content_type) const {
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos) {
        return "";
    }

    return content_type.substr(boundary_pos + 9);
}

std::string UploadHandler::parseMultipartFormData(const std::vector<char>& body, const std::string& boundary) {
    std::string data(body.begin(), body.end());
    std::string boundary_marker = "--" + boundary;

    // Find the filename
    size_t filename_pos = data.find("filename=\"");
    if (filename_pos == std::string::npos) {
        return "400 Bad Request: No filename found in multipart data";
    }

    filename_pos += 10; // Skip "filename=\""
    size_t filename_end = data.find("\"", filename_pos);
    if (filename_end == std::string::npos) {
        return "400 Bad Request: Malformed filename in multipart data";
    }

    std::string filename = data.substr(filename_pos, filename_end - filename_pos);

    // Validate file extension
    if (!isValidImageExtension(filename)) {
        return "400 Bad Request: Invalid file type. Only JPG, PNG, GIF, BMP allowed.";
    }

    // Find file data start
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

    // Find file data end
    size_t file_data_end = data.find(boundary_marker, file_data_start);
    if (file_data_end == std::string::npos) {
        file_data_end = data.length();
    }

    // Extract file data
    std::vector<char> file_data(data.begin() + file_data_start, data.begin() + file_data_end);

    // Save file
    std::string saved_path = saveFile(filename, file_data);

    // Return a simple success message for the client
    return "File uploaded successfully!";
}

std::string UploadHandler::saveFile(const std::string& filename, const std::vector<char>& data) {
    std::string safe_filename = generateFilename(filename);
    std::string full_path = upload_dir_ + "/" + safe_filename;

    std::ofstream file(full_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to create file: " + full_path);
    }

    file.write(data.data(), data.size());
    if (!file) {
        throw std::runtime_error("Failed to write to file: " + full_path);
    }

    return full_path;
}
