#include "Image.h"

#include <algorithm>

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
    for (size_t i = 0; i < pixels_.size(); i += 3) {
        pixels_[i] = r;
        pixels_[i + 1] = g;
        pixels_[i + 2] = b;
    }
}

std::string Image::toPPM() const {
    std::string header = "P6\n" + std::to_string(width_) + " " + std::to_string(height_) + "\n255\n";
    std::string data(pixels_.begin(), pixels_.end());
    return header + data;
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
    const int dataSize = (rowStride + padding) * height_;
    const int fileSize = 14 + 40 + dataSize;

    std::vector<unsigned char> bmp;
    bmp.reserve(fileSize);

    // File header
    bmp.push_back('B');
    bmp.push_back('M');
    writeInt32(bmp, fileSize);
    writeInt16(bmp, 0);
    writeInt16(bmp, 0);
    writeInt32(bmp, 54); // Pixel data offset

    // DIB header (BITMAPINFOHEADER)
    writeInt32(bmp, 40); // header size
    writeInt32(bmp, width_);
    writeInt32(bmp, height_);
    writeInt16(bmp, 1);  // planes
    writeInt16(bmp, 24); // bits per pixel
    writeInt32(bmp, 0);  // compression
    writeInt32(bmp, dataSize);
    writeInt32(bmp, 2835); // horizontal resolution (72 DPI)
    writeInt32(bmp, 2835); // vertical resolution
    writeInt32(bmp, 0);    // colors in palette
    writeInt32(bmp, 0);    // important colors

    // Pixel data (bottom-to-top)
    for (int y = height_ - 1; y >= 0; --y) {
        for (int x = 0; x < width_; ++x) {
            const size_t index = static_cast<size_t>(y) * width_ * 3 + static_cast<size_t>(x) * 3;
            const unsigned char r = pixels_[index];
            const unsigned char g = pixels_[index + 1];
            const unsigned char b = pixels_[index + 2];
            bmp.push_back(b);
            bmp.push_back(g);
            bmp.push_back(r);
        }
        for (int p = 0; p < padding; ++p) {
            bmp.push_back(0);
        }
    }

    return std::string(reinterpret_cast<const char*>(bmp.data()), bmp.size());
}
