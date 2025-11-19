#include "Image.h"

#include <algorithm>
#include <cstring>

Image::Image(int width, int height)
    : width_(width), height_(height), pixels_(static_cast<size_t>(width) * height * 3, 255) {}

void Image::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
    }
    const size_t index = static_cast<size_t>(y) * width_ * 3 + static_cast<size_t>(x) * 3;
    pixels_[index] = r;
    pixels_[index + 1] = g;
    pixels_[index + 2] = b;
}

void Image::clear(unsigned char r, unsigned char g, unsigned char b) {
    for (size_t i = 0; i < pixels_.size(); i += 3) {
        pixels_[i] = r;
        pixels_[i + 1] = g;
        pixels_[i + 2] = b;
    }
}

std::string Image::toPPM() const {
    std::string header = "P6\n" + std::to_string(width_) + " " + std::to_string(height_) + "\n255\n";
    std::string data;
    data.reserve(header.size() + pixels_.size());
    data.append(header);
    data.append(reinterpret_cast<const char *>(pixels_.data()), pixels_.size());
    return data;
}

std::string Image::toBMP() const {
    const int rowStride = ((width_ * 3 + 3) / 4) * 4;
    const int pixelDataSize = rowStride * height_;
    const int fileSize = 54 + pixelDataSize;

    std::vector<unsigned char> data;
    data.reserve(static_cast<size_t>(fileSize));

    // BMP file header
    data.push_back('B');
    data.push_back('M');
    writeInt32(data, fileSize);
    writeInt16(data, 0);
    writeInt16(data, 0);
    writeInt32(data, 54);

    // DIB header (BITMAPINFOHEADER)
    writeInt32(data, 40);
    writeInt32(data, width_);
    writeInt32(data, height_);
    writeInt16(data, 1);
    writeInt16(data, 24);
    writeInt32(data, 0);
    writeInt32(data, pixelDataSize);
    writeInt32(data, 2835); // 72 DPI
    writeInt32(data, 2835);
    writeInt32(data, 0);
    writeInt32(data, 0);

    std::vector<unsigned char> row;
    row.resize(static_cast<size_t>(rowStride));

    for (int y = height_ - 1; y >= 0; --y) {
        size_t rowIndex = 0;
        for (int x = 0; x < width_; ++x) {
            const size_t index = static_cast<size_t>(y) * width_ * 3 + static_cast<size_t>(x) * 3;
            const unsigned char r = pixels_[index];
            const unsigned char g = pixels_[index + 1];
            const unsigned char b = pixels_[index + 2];
            row[rowIndex++] = b;
            row[rowIndex++] = g;
            row[rowIndex++] = r;
        }
        while (rowIndex < row.size()) {
            row[rowIndex++] = 0;
        }
        data.insert(data.end(), row.begin(), row.end());
    }

    return std::string(reinterpret_cast<const char *>(data.data()), data.size());
}

void Image::writeInt32(std::vector<unsigned char> &data, int value) const {
    data.push_back(static_cast<unsigned char>(value & 0xFF));
    data.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    data.push_back(static_cast<unsigned char>((value >> 16) & 0xFF));
    data.push_back(static_cast<unsigned char>((value >> 24) & 0xFF));
}

void Image::writeInt16(std::vector<unsigned char> &data, short value) const {
    data.push_back(static_cast<unsigned char>(value & 0xFF));
    data.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
}
