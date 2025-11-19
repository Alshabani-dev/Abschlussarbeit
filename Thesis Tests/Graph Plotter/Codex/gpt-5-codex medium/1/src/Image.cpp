#include "Image.h"

#include <algorithm>
#include <cstring>

Image::Image(int width, int height)
    : width_(width), height_(height), pixels_(width * height * 3, 255) {}

void Image::clear(unsigned char r, unsigned char g, unsigned char b) {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            setPixel(x, y, r, g, b);
        }
    }
}

void Image::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
    }
    size_t index = (y * width_ + x) * 3;
    pixels_[index] = r;
    pixels_[index + 1] = g;
    pixels_[index + 2] = b;
}

std::string Image::toPPM() const {
    std::string header = "P6\n" + std::to_string(width_) + " " + std::to_string(height_) + "\n255\n";
    std::string data;
    data.reserve(header.size() + pixels_.size());
    data += header;
    data.append(reinterpret_cast<const char*>(pixels_.data()), pixels_.size());
    return data;
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
    const int rowSize = ((24 * width_ + 31) / 32) * 4;
    const int pixelDataSize = rowSize * height_;
    const int fileSize = 54 + pixelDataSize;

    std::vector<unsigned char> bmp;
    bmp.reserve(fileSize);

    // BMP Header
    bmp.push_back('B');
    bmp.push_back('M');
    writeInt32(bmp, fileSize);
    writeInt16(bmp, 0);
    writeInt16(bmp, 0);
    writeInt32(bmp, 54);

    // DIB Header (BITMAPINFOHEADER)
    writeInt32(bmp, 40);
    writeInt32(bmp, width_);
    writeInt32(bmp, height_);
    writeInt16(bmp, 1);
    writeInt16(bmp, 24);
    writeInt32(bmp, 0);
    writeInt32(bmp, pixelDataSize);
    writeInt32(bmp, 2835);
    writeInt32(bmp, 2835);
    writeInt32(bmp, 0);
    writeInt32(bmp, 0);

    std::vector<unsigned char> row(rowSize);
    for (int y = height_ - 1; y >= 0; --y) {
        std::fill(row.begin(), row.end(), 0);
        for (int x = 0; x < width_; ++x) {
            const size_t srcIndex = (y * width_ + x) * 3;
            const size_t dstIndex = x * 3;
            row[dstIndex] = pixels_[srcIndex + 2];
            row[dstIndex + 1] = pixels_[srcIndex + 1];
            row[dstIndex + 2] = pixels_[srcIndex];
        }
        bmp.insert(bmp.end(), row.begin(), row.end());
    }

    return std::string(reinterpret_cast<const char*>(bmp.data()), bmp.size());
}
