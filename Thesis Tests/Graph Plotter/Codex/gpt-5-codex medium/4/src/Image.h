#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <vector>

class Image {
public:
    Image(int width, int height);

    void clear(unsigned char r, unsigned char g, unsigned char b);
    void setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);

    std::string toPPM() const;
    std::string toBMP() const;

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    int width_;
    int height_;
    std::vector<unsigned char> pixels_; // RGB order

    void writeInt16(std::vector<unsigned char> &buffer, short value) const;
    void writeInt32(std::vector<unsigned char> &buffer, int value) const;
};

#endif
