#include "Image.h"

#include <algorithm>
#include <sstream>

Image::Image(int width, int height)
    : width_(width), height_(height), pixels_(width * height * 3, 255) {}

void Image::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;
    }
    const size_t index = static_cast<size_t>(y) * width_ * 3 + static_cast<size_t>(x) * 3;
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
    std::ostringstream buffer;
    buffer << "P6\n" << width_ << " " << height_ << "\n255\n";
    buffer.write(reinterpret_cast<const char*>(pixels_.data()), pixels_.size());
    return buffer.str();
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
    const int rowStride = width_ * 3;
    const int padding = (4 - (rowStride % 4)) % 4;
    const int imageSize = (rowStride + padding) * height_;
    const int fileSize = 14 + 40 + imageSize;

    std::vector<unsigned char> data;
    data.reserve(fileSize);

    data.push_back('B');
    data.push_back('M');
    writeInt32(data, fileSize);
    writeInt16(data, 0);
    writeInt16(data, 0);
    writeInt32(data, 14 + 40);

    writeInt32(data, 40);
    writeInt32(data, width_);
    writeInt32(data, height_);
    writeInt16(data, 1);
    writeInt16(data, 24);
    writeInt32(data, 0);
    writeInt32(data, imageSize);
    writeInt32(data, 2835);
    writeInt32(data, 2835);
    writeInt32(data, 0);
    writeInt32(data, 0);

    std::vector<unsigned char> paddingBytes(padding, 0);
    for (int y = height_ - 1; y >= 0; --y) {
        for (int x = 0; x < width_; ++x) {
            const size_t index = static_cast<size_t>(y) * width_ * 3 + static_cast<size_t>(x) * 3;
            const unsigned char r = pixels_[index];
            const unsigned char g = pixels_[index + 1];
            const unsigned char b = pixels_[index + 2];
            data.push_back(b);
            data.push_back(g);
            data.push_back(r);
        }
        data.insert(data.end(), paddingBytes.begin(), paddingBytes.end());
    }

    return std::string(data.begin(), data.end());
}
