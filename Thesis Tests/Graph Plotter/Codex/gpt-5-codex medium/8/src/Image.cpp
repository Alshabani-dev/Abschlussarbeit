#include "Image.h"

#include <cstring>

Image::Image(int width, int height)
    : width_(width),
      height_(height),
      pixels_(width * height * 3, 255) {}

void Image::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
    }
    size_t index = static_cast<size_t>(y) * width_ * 3 + static_cast<size_t>(x) * 3;
    pixels_[index + 0] = r;
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
    std::string data;
    std::string header = "P6\n" + std::to_string(width_) + " " + std::to_string(height_) + "\n255\n";
    data.reserve(header.size() + pixels_.size());
    data.append(header);
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
    const int rowStride = width_ * 3;
    const int padding = (4 - (rowStride % 4)) % 4;
    const int paddedStride = rowStride + padding;
    const int pixelArraySize = paddedStride * height_;
    const int fileHeaderSize = 14;
    const int dibHeaderSize = 40;
    const int fileSize = fileHeaderSize + dibHeaderSize + pixelArraySize;

    std::vector<unsigned char> pixelData(pixelArraySize, 0);
    for (int y = 0; y < height_; ++y) {
        unsigned char *dstRow = pixelData.data() + y * paddedStride;
        int srcY = height_ - 1 - y;
        for (int x = 0; x < width_; ++x) {
            size_t srcIndex = static_cast<size_t>(srcY) * width_ * 3 + static_cast<size_t>(x) * 3;
            dstRow[x * 3 + 0] = pixels_[srcIndex + 2];
            dstRow[x * 3 + 1] = pixels_[srcIndex + 1];
            dstRow[x * 3 + 2] = pixels_[srcIndex + 0];
        }
    }

    std::vector<unsigned char> data;
    data.reserve(fileSize);

    // File header
    data.push_back('B');
    data.push_back('M');
    writeInt32(data, fileSize);
    writeInt16(data, 0);
    writeInt16(data, 0);
    writeInt32(data, fileHeaderSize + dibHeaderSize);

    // DIB header (BITMAPINFOHEADER)
    writeInt32(data, dibHeaderSize);
    writeInt32(data, width_);
    writeInt32(data, height_);
    writeInt16(data, 1);               // planes
    writeInt16(data, 24);              // bits per pixel
    writeInt32(data, 0);               // compression
    writeInt32(data, pixelArraySize);  // image size
    writeInt32(data, 2835);            // horizontal resolution (~72 DPI)
    writeInt32(data, 2835);            // vertical resolution
    writeInt32(data, 0);               // colors in palette
    writeInt32(data, 0);               // important colors

    data.insert(data.end(), pixelData.begin(), pixelData.end());

    return std::string(reinterpret_cast<char *>(data.data()), data.size());
}
