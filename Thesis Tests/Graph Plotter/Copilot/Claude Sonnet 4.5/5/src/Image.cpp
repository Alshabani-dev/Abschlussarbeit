#include "Image.h"
#include <algorithm>
#include <sstream>

Image::Image(int width, int height) 
    : width_(width), height_(height) {
    pixels_.resize(width * height * 3, 0);
}

void Image::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    // Clamp coordinates to image bounds
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;
    }
    
    int index = (y * width_ + x) * 3;
    pixels_[index] = r;
    pixels_[index + 1] = g;
    pixels_[index + 2] = b;
}

void Image::clear(unsigned char r, unsigned char g, unsigned char b) {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            setPixel(x, y, r, g, b);
        }
    }
}

std::string Image::toPPM() const {
    std::stringstream ss;
    ss << "P6\n" << width_ << " " << height_ << "\n255\n";
    
    std::string header = ss.str();
    std::string result;
    result.reserve(header.size() + pixels_.size());
    result = header;
    result.append(reinterpret_cast<const char*>(pixels_.data()), pixels_.size());
    
    return result;
}

void Image::writeInt32(std::vector<unsigned char>& data, int value) const {
    // Write 32-bit integer in little-endian format
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
    data.push_back((value >> 16) & 0xFF);
    data.push_back((value >> 24) & 0xFF);
}

void Image::writeInt16(std::vector<unsigned char>& data, short value) const {
    // Write 16-bit integer in little-endian format
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
}

std::string Image::toBMP() const {
    std::vector<unsigned char> bmpData;
    
    // Calculate row padding (rows must be aligned to 4-byte boundaries)
    int rowPadding = (4 - (width_ * 3) % 4) % 4;
    int rowSize = width_ * 3 + rowPadding;
    int pixelDataSize = rowSize * height_;
    int fileSize = 54 + pixelDataSize;
    
    // BMP File Header (14 bytes)
    bmpData.push_back('B');
    bmpData.push_back('M');
    writeInt32(bmpData, fileSize);        // File size
    writeInt32(bmpData, 0);               // Reserved
    writeInt32(bmpData, 54);              // Pixel data offset
    
    // DIB Header (BITMAPINFOHEADER - 40 bytes)
    writeInt32(bmpData, 40);              // Header size
    writeInt32(bmpData, width_);          // Width
    writeInt32(bmpData, height_);         // Height
    writeInt16(bmpData, 1);               // Color planes
    writeInt16(bmpData, 24);              // Bits per pixel
    writeInt32(bmpData, 0);               // Compression (none)
    writeInt32(bmpData, pixelDataSize);   // Image size
    writeInt32(bmpData, 2835);            // X pixels per meter
    writeInt32(bmpData, 2835);            // Y pixels per meter
    writeInt32(bmpData, 0);               // Colors used
    writeInt32(bmpData, 0);               // Important colors
    
    // Pixel data (BGR format, bottom-to-top)
    for (int y = height_ - 1; y >= 0; --y) {
        for (int x = 0; x < width_; ++x) {
            int index = (y * width_ + x) * 3;
            bmpData.push_back(pixels_[index + 2]); // B
            bmpData.push_back(pixels_[index + 1]); // G
            bmpData.push_back(pixels_[index]);     // R
        }
        // Add padding bytes
        for (int p = 0; p < rowPadding; ++p) {
            bmpData.push_back(0);
        }
    }
    
    return std::string(reinterpret_cast<const char*>(bmpData.data()), bmpData.size());
}
