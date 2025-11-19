#include "Image.h"

#include <algorithm>
#include <stdexcept>

Image::Image(int width, int height)
    : width_(width), height_(height), pixels_(width * height * 3, 255) {
    if (width_ <= 0 || height_ <= 0) {
        throw std::runtime_error("Invalid image dimensions");
    }
}

void Image::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
    }
    size_t index = static_cast<size_t>((y * width_ + x) * 3);
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
    std::string header = "P6\n" + std::to_string(width_) + " " + std::to_string(height_) + "\n255\n";
    std::string data = header;
    data.append(reinterpret_cast<const char *>(pixels_.data()), pixels_.size());
    return data;
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

std::string Image::toBMP() const {
    const int bytesPerPixel = 3;
    const int rowStride = width_ * bytesPerPixel;
    const int padding = (4 - (rowStride % 4)) % 4;
    const int dataSize = (rowStride + padding) * height_;
    const int fileSize = 14 + 40 + dataSize;

    std::vector<unsigned char> buffer;
    buffer.reserve(fileSize);

    // BMP file header
    buffer.push_back('B');
    buffer.push_back('M');
    writeInt32(buffer, fileSize);
    writeInt16(buffer, 0);
    writeInt16(buffer, 0);
    writeInt32(buffer, 14 + 40);

    // DIB header (BITMAPINFOHEADER)
    writeInt32(buffer, 40);
    writeInt32(buffer, width_);
    writeInt32(buffer, height_);
    writeInt16(buffer, 1);
    writeInt16(buffer, 24);
    writeInt32(buffer, 0);
    writeInt32(buffer, dataSize);
    writeInt32(buffer, 2835);
    writeInt32(buffer, 2835);
    writeInt32(buffer, 0);
    writeInt32(buffer, 0);

    std::vector<unsigned char> row(rowStride + padding, 0);
    for (int y = height_ - 1; y >= 0; --y) {
        for (int x = 0; x < width_; ++x) {
            size_t srcIndex = static_cast<size_t>((y * width_ + x) * 3);
            size_t dstIndex = static_cast<size_t>(x * 3);
            row[dstIndex] = pixels_[srcIndex + 2];
            row[dstIndex + 1] = pixels_[srcIndex + 1];
            row[dstIndex + 2] = pixels_[srcIndex];
        }
        buffer.insert(buffer.end(), row.begin(), row.begin() + rowStride + padding);
    }

    return std::string(reinterpret_cast<const char *>(buffer.data()), buffer.size());
}
