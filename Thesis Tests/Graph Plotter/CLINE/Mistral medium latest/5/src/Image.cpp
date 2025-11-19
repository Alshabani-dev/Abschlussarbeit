#include "Image.h"
#include <algorithm>
#include <cstring>

Image::Image(int width, int height) : width_(width), height_(height) {
    pixels_.resize(width * height * 3, 0); // 3 bytes per pixel (RGB)
}

void Image::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        int index = (y * width_ + x) * 3;
        pixels_[index] = r;
        pixels_[index + 1] = g;
        pixels_[index + 2] = b;
    }
}

void Image::clear(unsigned char r, unsigned char g, unsigned char b) {
    for (size_t i = 0; i < pixels_.size(); i += 3) {
        pixels_[i] = r;
        pixels_[i + 1] = g;
        pixels_[i + 2] = b;
    }
}

std::string Image::toPPM() const {
    std::string ppm = "P6\n" + std::to_string(width_) + " " + std::to_string(height_) + "\n255\n";
    std::vector<unsigned char> ppmData(ppm.begin(), ppm.end());
    ppmData.insert(ppmData.end(), pixels_.begin(), pixels_.end());
    return std::string(ppmData.begin(), ppmData.end());
}

void Image::writeInt32(std::vector<unsigned char>& data, int value) const {
    data.push_back(static_cast<unsigned char>(value & 0xFF));
    data.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    data.push_back(static_cast<unsigned char>((value >> 16) & 0xFF));
    data.push_back(static_cast<unsigned char>((value >> 24) & 0xFF));
}

void Image::writeInt16(std::vector<unsigned char>& data, short value) const {
    data.push_back(static_cast<unsigned char>(value & 0xFF));
    data.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
}

std::string Image::toBMP() const {
    std::vector<unsigned char> bmpData;

    // Calculate row padding
    int rowPadding = (4 - (width_ * 3) % 4) % 4;
    int rowSize = width_ * 3 + rowPadding;
    int pixelDataSize = rowSize * height_;
    int fileSize = 54 + pixelDataSize;

    // Write file header (14 bytes)
    bmpData.push_back('B');
    bmpData.push_back('M');
    writeInt32(bmpData, fileSize);
    writeInt32(bmpData, 0); // Reserved
    writeInt32(bmpData, 54); // Pixel data offset

    // Write DIB header (40 bytes)
    writeInt32(bmpData, 40); // Header size
    writeInt32(bmpData, width_);
    writeInt32(bmpData, height_);
    writeInt16(bmpData, 1); // Color planes
    writeInt16(bmpData, 24); // Bits per pixel
    writeInt32(bmpData, 0); // Compression
    writeInt32(bmpData, pixelDataSize); // Image size
    writeInt32(bmpData, 0); // X pixels per meter
    writeInt32(bmpData, 0); // Y pixels per meter
    writeInt32(bmpData, 0); // Colors used
    writeInt32(bmpData, 0); // Important colors

    // Write pixels: bottom-to-top, BGR format
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
