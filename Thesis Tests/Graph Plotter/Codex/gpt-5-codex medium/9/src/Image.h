#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include <string>

class Image {
public:
    Image(int width, int height);

    void setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);
    void clear(unsigned char r, unsigned char g, unsigned char b);

    std::string toPPM() const;
    std::string toBMP() const;

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    int width_;
    int height_;
    std::vector<unsigned char> pixels_;

    void writeInt32(std::vector<unsigned char>& data, int value) const;
    void writeInt16(std::vector<unsigned char>& data, short value) const;
};

#endif
