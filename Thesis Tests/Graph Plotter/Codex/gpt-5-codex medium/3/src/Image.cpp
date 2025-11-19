#include "Image.h"

Image::Image(int width, int height)
    : width_(width), height_(height), pixels_(width * height * 3, 255) {}

void Image::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;
    }
    const size_t index = static_cast<size_t>(y * width_ + x) * 3;
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
    const int rowStride = width_ * 3;
    const int padding = (4 - (rowStride % 4)) % 4;
    const int bmpDataSize = (rowStride + padding) * height_;
    const int fileSize = 14 + 40 + bmpDataSize;

    std::vector<unsigned char> bmp;
    bmp.reserve(fileSize);

    // BMP Header
    bmp.push_back('B');
    bmp.push_back('M');
    writeInt32(bmp, fileSize);
    writeInt16(bmp, 0);
    writeInt16(bmp, 0);
    writeInt32(bmp, 54); // Pixel data offset

    // DIB Header (BITMAPINFOHEADER)
    writeInt32(bmp, 40);         // Header size
    writeInt32(bmp, width_);
    writeInt32(bmp, height_);
    writeInt16(bmp, 1);          // Color planes
    writeInt16(bmp, 24);         // Bits per pixel
    writeInt32(bmp, 0);          // Compression
    writeInt32(bmp, bmpDataSize);
    writeInt32(bmp, 2835);       // Horizontal resolution (72 DPI)
    writeInt32(bmp, 2835);       // Vertical resolution
    writeInt32(bmp, 0);          // Colors used
    writeInt32(bmp, 0);          // Important colors

    const unsigned char pad[3] = {0, 0, 0};
    for (int y = height_ - 1; y >= 0; --y) { // BMP rows bottom-to-top
        for (int x = 0; x < width_; ++x) {
            const size_t idx = static_cast<size_t>(y * width_ + x) * 3;
            unsigned char r = pixels_[idx];
            unsigned char g = pixels_[idx + 1];
            unsigned char b = pixels_[idx + 2];
            bmp.push_back(b);
            bmp.push_back(g);
            bmp.push_back(r);
        }
        bmp.insert(bmp.end(), pad, pad + padding);
    }

    return std::string(reinterpret_cast<const char*>(bmp.data()), bmp.size());
}
