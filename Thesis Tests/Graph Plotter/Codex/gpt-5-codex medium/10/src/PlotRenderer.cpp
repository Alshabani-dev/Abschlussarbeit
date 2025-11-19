#include "PlotRenderer.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace {

const unsigned char kBackground = 255;
const unsigned char kTextColor = 40;
const unsigned char kAxisColor = 80;
const unsigned char kLineColorR = 40;
const unsigned char kLineColorG = 90;
const unsigned char kLineColorB = 160;
const unsigned char kScatterColorR = 220;
const unsigned char kScatterColorG = 60;
const unsigned char kScatterColorB = 80;
const unsigned char kBarColorR = 70;
const unsigned char kBarColorG = 160;
const unsigned char kBarColorB = 100;

const int digitWidth = 3;
const int digitHeight = 5;

const int digitFont[12][digitWidth * digitHeight] = {
    {1,1,1,
     1,0,1,
     1,0,1,
     1,0,1,
     1,1,1}, // 0
    {0,1,0,
     1,1,0,
     0,1,0,
     0,1,0,
     1,1,1}, // 1
    {1,1,1,
     0,0,1,
     1,1,1,
     1,0,0,
     1,1,1}, // 2
    {1,1,1,
     0,0,1,
     0,1,1,
     0,0,1,
     1,1,1}, // 3
    {1,0,1,
     1,0,1,
     1,1,1,
     0,0,1,
     0,0,1}, // 4
    {1,1,1,
     1,0,0,
     1,1,1,
     0,0,1,
     1,1,1}, // 5
    {1,1,1,
     1,0,0,
     1,1,1,
     1,0,1,
     1,1,1}, // 6
    {1,1,1,
     0,0,1,
     0,1,0,
     0,1,0,
     0,1,0}, // 7
    {1,1,1,
     1,0,1,
     1,1,1,
     1,0,1,
     1,1,1}, // 8
    {1,1,1,
     1,0,1,
     1,1,1,
     0,0,1,
     1,1,1}, // 9
    {0,0,0,
     0,0,0,
     1,1,1,
     0,0,0,
     0,0,0}, // -
    {0,0,0,
     0,0,0,
     0,1,0,
     0,0,0,
     0,0,0}  // .
};

}

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width),
      height_(height),
      marginLeft_(60),
      marginRight_(20),
      marginTop_(30),
      marginBottom_(60) {}

Image PlotRenderer::renderLinePlot(const std::vector<double> &xs, const std::vector<double> &ys) {
    Image image(width_, height_);
    image.clear(kBackground, kBackground, kBackground);

    if (xs.empty() || ys.empty() || xs.size() != ys.size()) {
        return image;
    }

    double minX, maxX, minY, maxY;
    findMinMax(xs, minX, maxX);
    findMinMax(ys, minY, maxY);

    drawAxes(image, minX, maxX, minY, maxY);
    drawGrid(image, minX, maxX, minY, maxY);

    int prevX = mapX(xs[0], minX, maxX);
    int prevY = mapY(ys[0], minY, maxY);

    for (size_t i = 1; i < xs.size(); ++i) {
        int newX = mapX(xs[i], minX, maxX);
        int newY = mapY(ys[i], minY, maxY);
        drawLine(image, prevX, prevY, newX, newY, kLineColorR, kLineColorG, kLineColorB);
        prevX = newX;
        prevY = newY;
    }

    return image;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double> &xs, const std::vector<double> &ys) {
    Image image(width_, height_);
    image.clear(kBackground, kBackground, kBackground);

    if (xs.empty() || ys.empty() || xs.size() != ys.size()) {
        return image;
    }

    double minX, maxX, minY, maxY;
    findMinMax(xs, minX, maxX);
    findMinMax(ys, minY, maxY);

    drawAxes(image, minX, maxX, minY, maxY);
    drawGrid(image, minX, maxX, minY, maxY);

    for (size_t i = 0; i < xs.size(); ++i) {
        int px = mapX(xs[i], minX, maxX);
        int py = mapY(ys[i], minY, maxY);
        drawCircle(image, px, py, 3, kScatterColorR, kScatterColorG, kScatterColorB);
    }

    return image;
}

Image PlotRenderer::renderBarPlot(const std::vector<double> &xs, const std::vector<double> &ys) {
    Image image(width_, height_);
    image.clear(kBackground, kBackground, kBackground);

    if (xs.empty() || ys.empty() || xs.size() != ys.size()) {
        return image;
    }

    double minX, maxX, minY, maxY;
    findMinMax(xs, minX, maxX);
    findMinMax(ys, minY, maxY);

    drawAxes(image, minX, maxX, minY, maxY);
    drawGrid(image, minX, maxX, minY, maxY);

    std::vector<int> pixelPositions;
    pixelPositions.reserve(xs.size());
    for (double x : xs) {
        pixelPositions.push_back(mapX(x, minX, maxX));
    }

    int barHalfWidth = 8;
    if (pixelPositions.size() > 1) {
        int minDiff = width_;
        for (size_t i = 1; i < pixelPositions.size(); ++i) {
            minDiff = std::min(minDiff, std::abs(pixelPositions[i] - pixelPositions[i - 1]));
        }
        barHalfWidth = std::max(4, minDiff / 3);
    }

    int zeroY = mapY(0.0, minY, maxY);
    int plotTop = marginTop_;
    int plotBottom = height_ - marginBottom_;
    zeroY = std::min(std::max(zeroY, plotTop), plotBottom);

    for (size_t i = 0; i < xs.size(); ++i) {
        int px = pixelPositions[i];
        int py = mapY(ys[i], minY, maxY);
        drawSquare(image, px - barHalfWidth, py, px + barHalfWidth, zeroY,
                   kBarColorR, kBarColorG, kBarColorB);
    }

    return image;
}

void PlotRenderer::findMinMax(const std::vector<double> &data, double &minVal, double &maxVal) const {
    minVal = data.front();
    maxVal = data.front();
    for (double value : data) {
        minVal = std::min(minVal, value);
        maxVal = std::max(maxVal, value);
    }
    if (minVal == maxVal) {
        minVal -= 1.0;
        maxVal += 1.0;
    }
}

int PlotRenderer::mapX(double value, double minVal, double maxVal) const {
    double plotWidth = static_cast<double>(width_ - marginLeft_ - marginRight_);
    double ratio = (value - minVal) / (maxVal - minVal);
    ratio = std::max(0.0, std::min(1.0, ratio));
    return marginLeft_ + static_cast<int>(ratio * plotWidth + 0.5);
}

int PlotRenderer::mapY(double value, double minVal, double maxVal) const {
    double plotHeight = static_cast<double>(height_ - marginTop_ - marginBottom_);
    double ratio = (value - minVal) / (maxVal - minVal);
    ratio = std::max(0.0, std::min(1.0, ratio));
    double pixel = marginTop_ + (1.0 - ratio) * plotHeight;
    return static_cast<int>(pixel + 0.5);
}

void PlotRenderer::drawLine(Image &image, int x0, int y0, int x1, int y1,
                            unsigned char r, unsigned char g, unsigned char b) {
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        image.setPixel(x0, y0, r, g, b);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void PlotRenderer::drawCircle(Image &image, int cx, int cy, int radius,
                              unsigned char r, unsigned char g, unsigned char b) {
    int radiusSq = radius * radius;
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radiusSq) {
                image.setPixel(cx + x, cy + y, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image &image, int x0, int y0, int x1, int y1,
                              unsigned char r, unsigned char g, unsigned char b) {
    if (x0 > x1) {
        std::swap(x0, x1);
    }
    if (y0 > y1) {
        std::swap(y0, y1);
    }
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            image.setPixel(x, y, r, g, b);
        }
    }
}

void PlotRenderer::drawDigit(Image &image, int x, int y, char digit,
                             unsigned char r, unsigned char g, unsigned char b) {
    int index = -1;
    if (digit >= '0' && digit <= '9') {
        index = digit - '0';
    } else if (digit == '-') {
        index = 10;
    } else if (digit == '.') {
        index = 11;
    }

    if (index < 0) {
        return;
    }

    for (int row = 0; row < digitHeight; ++row) {
        for (int col = 0; col < digitWidth; ++col) {
            if (digitFont[index][row * digitWidth + col]) {
                image.setPixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawText(Image &image, int x, int y, const std::string &text,
                            unsigned char r, unsigned char g, unsigned char b) {
    int cursorX = x;
    for (char c : text) {
        drawDigit(image, cursorX, y, c, r, g, b);
        cursorX += digitWidth + 1;
    }
}

void PlotRenderer::drawAxes(Image &image, double minX, double maxX, double minY, double maxY) {
    int left = marginLeft_;
    int right = width_ - marginRight_;
    int top = marginTop_;
    int bottom = height_ - marginBottom_;

    drawLine(image, left, top, left, bottom, kAxisColor, kAxisColor, kAxisColor);
    drawLine(image, left, bottom, right, bottom, kAxisColor, kAxisColor, kAxisColor);
    drawLine(image, right, top, right, bottom, kAxisColor, kAxisColor, kAxisColor);
    drawLine(image, left, top, right, top, kAxisColor, kAxisColor, kAxisColor);

    int textY = bottom + 10;
    drawText(image, left, textY, std::to_string(static_cast<int>(std::round(minX))),
             kTextColor, kTextColor, kTextColor);
    drawText(image, right - 20, textY, std::to_string(static_cast<int>(std::round(maxX))),
             kTextColor, kTextColor, kTextColor);

    drawText(image, left - 25, top, std::to_string(static_cast<int>(std::round(maxY))),
             kTextColor, kTextColor, kTextColor);
    drawText(image, left - 25, bottom - 10, std::to_string(static_cast<int>(std::round(minY))),
             kTextColor, kTextColor, kTextColor);
}

void PlotRenderer::drawGrid(Image &image, double minX, double maxX, double minY, double maxY) {
    int left = marginLeft_;
    int right = width_ - marginRight_;
    int top = marginTop_;
    int bottom = height_ - marginBottom_;

    int minXInt = static_cast<int>(std::ceil(minX));
    int maxXInt = static_cast<int>(std::floor(maxX));
    for (int value = minXInt; value <= maxXInt; ++value) {
        int px = mapX(static_cast<double>(value), minX, maxX);
        for (int y = top; y <= bottom; ++y) {
            image.setPixel(px, y, 220, 220, 220);
        }
        drawText(image, px - 4, bottom + 20, std::to_string(value), kTextColor, kTextColor, kTextColor);
    }

    int minYInt = static_cast<int>(std::ceil(minY));
    int maxYInt = static_cast<int>(std::floor(maxY));
    for (int value = minYInt; value <= maxYInt; ++value) {
        int py = mapY(static_cast<double>(value), minY, maxY);
        for (int x = left; x <= right; ++x) {
            image.setPixel(x, py, 220, 220, 220);
        }
        drawText(image, left - 30, py - 4, std::to_string(value), kTextColor, kTextColor, kTextColor);
    }
}
