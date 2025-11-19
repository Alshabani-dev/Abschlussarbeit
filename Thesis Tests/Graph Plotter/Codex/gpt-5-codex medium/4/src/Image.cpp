#include "Image.h"

#include <algorithm>
#include <stdexcept>

Image::Image(int width, int height)
    : width_(width), height_(height), pixels_(width * height * 3, 255) {
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Invalid image dimensions");
    }
}

void Image::clear(unsigned char r, unsigned char g, unsigned char b) {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            setPixel(x, y, r, g, b);
        }
    }
}

void Image::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;
    }
    size_t index = static_cast<size_t>(y * width_ + x) * 3;
    pixels_[index] = r;
    pixels_[index + 1] = g;
    pixels_[index + 2] = b;
}

std::string Image::toPPM() const {
    std::string header = "P6\n" + std::to_string(width_) + " " + std::to_string(height_) + "\n255\n";
    std::string data;
    data.reserve(header.size() + pixels_.size());
    data.append(header);
    data.append(reinterpret_cast<const char *>(pixels_.data()), pixels_.size());
    return data;
}

void Image::writeInt16(std::vector<unsigned char> &buffer, short value) const {
    buffer.push_back(static_cast<unsigned char>(value & 0xFF));
    buffer.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
}

void Image::writeInt32(std::vector<unsigned char> &buffer, int value) const {
    buffer.push_back(static_cast<unsigned char>(value & 0xFF));
    buffer.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<unsigned char>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<unsigned char>((value >> 24) & 0xFF));
}

std::string Image::toBMP() const {
    const int rowStride = width_ * 3;
    const int padding = (4 - (rowStride % 4)) % 4;
    const int dataSize = (rowStride + padding) * height_;
    const int headerSize = 14 + 40;
    const int fileSize = headerSize + dataSize;

    std::vector<unsigned char> buffer;
    buffer.reserve(fileSize);

    // BMP file header
    buffer.push_back('B');
    buffer.push_back('M');
    writeInt32(buffer, fileSize);
    writeInt16(buffer, 0);
    writeInt16(buffer, 0);
    writeInt32(buffer, headerSize);

    // DIB header (BITMAPINFOHEADER)
    writeInt32(buffer, 40);           // header size
    writeInt32(buffer, width_);
    writeInt32(buffer, height_);
    writeInt16(buffer, 1);            // planes
    writeInt16(buffer, 24);           // bits per pixel
    writeInt32(buffer, 0);            // compression
    writeInt32(buffer, dataSize);
    writeInt32(buffer, 2835);         // horizontal resolution (pixels per meter)
    writeInt32(buffer, 2835);         // vertical resolution
    writeInt32(buffer, 0);            // colors in palette
    writeInt32(buffer, 0);            // important colors

    // Pixel data (bottom to top)
    for (int y = height_ - 1; y >= 0; --y) {
        for (int x = 0; x < width_; ++x) {
            size_t index = static_cast<size_t>(y * width_ + x) * 3;
            unsigned char r = pixels_[index];
            unsigned char g = pixels_[index + 1];
            unsigned char b = pixels_[index + 2];
            buffer.push_back(b);
            buffer.push_back(g);
            buffer.push_back(r);
        }
        for (int p = 0; p < padding; ++p) {
            buffer.push_back(0);
        }
    }

    return std::string(reinterpret_cast<const char *>(buffer.data()), buffer.size());
}
