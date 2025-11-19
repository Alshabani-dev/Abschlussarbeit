#include "PlotRenderer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width),
      height_(height),
      marginLeft_(60),
      marginRight_(40),
      marginTop_(40),
      marginBottom_(60) {}

PlotRenderer::Range PlotRenderer::findRange(const std::vector<double>& values) const {
    if (values.empty()) {
        return {0.0, 1.0};
    }
    double minVal = values.front();
    double maxVal = values.front();
    for (double v : values) {
        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
    }
    if (std::abs(maxVal - minVal) < 1e-6) {
        maxVal += 1.0;
        minVal -= 1.0;
    }
    return {minVal, maxVal};
}

int PlotRenderer::mapX(double value, const Range& range) const {
    double t = (value - range.first) / (range.second - range.first);
    t = std::clamp(t, 0.0, 1.0);
    int plotWidth = width_ - marginLeft_ - marginRight_;
    return marginLeft_ + static_cast<int>(t * plotWidth + 0.5);
}

int PlotRenderer::mapY(double value, const Range& range) const {
    double t = (value - range.first) / (range.second - range.first);
    t = std::clamp(t, 0.0, 1.0);
    int plotHeight = height_ - marginTop_ - marginBottom_;
    return height_ - marginBottom_ - static_cast<int>(t * plotHeight + 0.5);
}

void PlotRenderer::drawLine(Image& image, int x0, int y0, int x1, int y1,
                            unsigned char r, unsigned char g, unsigned char b) const {
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        image.setPixel(x0, y0, r, g, b);
        if (x0 == x1 && y0 == y1) break;
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

void PlotRenderer::drawCircle(Image& image, int cx, int cy, int radius,
                              unsigned char r, unsigned char g, unsigned char b) const {
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                image.setPixel(cx + x, cy + y, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image& image, int cx, int cy, int size,
                              unsigned char r, unsigned char g, unsigned char b) const {
    int half = size / 2;
    for (int y = -half; y <= half; ++y) {
        for (int x = -half; x <= half; ++x) {
            image.setPixel(cx + x, cy + y, r, g, b);
        }
    }
}

void PlotRenderer::drawDigit(Image& image, int x, int y, int digit,
                             unsigned char r, unsigned char g, unsigned char b) const {
    static const int FONT[10][15] = {
        {1,1,1,
         1,0,1,
         1,0,1,
         1,0,1,
         1,1,1},
        {0,1,0,
         1,1,0,
         0,1,0,
         0,1,0,
         1,1,1},
        {1,1,1,
         0,0,1,
         1,1,1,
         1,0,0,
         1,1,1},
        {1,1,1,
         0,0,1,
         0,1,1,
         0,0,1,
         1,1,1},
        {1,0,1,
         1,0,1,
         1,1,1,
         0,0,1,
         0,0,1},
        {1,1,1,
         1,0,0,
         1,1,1,
         0,0,1,
         1,1,1},
        {1,1,1,
         1,0,0,
         1,1,1,
         1,0,1,
         1,1,1},
        {1,1,1,
         0,0,1,
         0,1,0,
         0,1,0,
         0,1,0},
        {1,1,1,
         1,0,1,
         1,1,1,
         1,0,1,
         1,1,1},
        {1,1,1,
         1,0,1,
         1,1,1,
         0,0,1,
         0,0,1}
    };

    if (digit < 0 || digit > 9) {
        return;
    }
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (FONT[digit][row * 3 + col]) {
                image.setPixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawText(Image& image, int x, int y, const std::string& text,
                            unsigned char r, unsigned char g, unsigned char b) const {
    int cursor = x;
    for (char c : text) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            drawDigit(image, cursor, y, c - '0', r, g, b);
            cursor += 4;
        } else if (c == '-') {
            for (int i = 0; i < 3; ++i) {
                image.setPixel(cursor + i, y + 2, r, g, b);
            }
            cursor += 4;
        } else if (c == '.') {
            image.setPixel(cursor + 1, y + 4, r, g, b);
            cursor += 3;
        } else if (c == ' ') {
            cursor += 3;
        } else {
            cursor += 3;
        }
    }
}

void PlotRenderer::drawAxes(Image& image, const Range& xRange, const Range& yRange) const {
    const int left = marginLeft_;
    const int right = width_ - marginRight_;
    const int top = marginTop_;
    const int bottom = height_ - marginBottom_;
    for (int x = left; x <= right; ++x) {
        image.setPixel(x, bottom, 0, 0, 0);
        image.setPixel(x, top, 0, 0, 0);
    }
    for (int y = top; y <= bottom; ++y) {
        image.setPixel(left, y, 0, 0, 0);
        image.setPixel(right, y, 0, 0, 0);
    }
    (void)xRange;
    (void)yRange;
}

void PlotRenderer::drawGrid(Image& image, const Range& xRange, const Range& yRange) const {
    const int left = marginLeft_;
    const int right = width_ - marginRight_;
    const int top = marginTop_;
    const int bottom = height_ - marginBottom_;

    int xStart = static_cast<int>(std::ceil(xRange.first));
    int xEnd = static_cast<int>(std::floor(xRange.second));
    for (int v = xStart; v <= xEnd; ++v) {
        int x = mapX(v, xRange);
        for (int y = top; y <= bottom; ++y) {
            image.setPixel(x, y, 220, 220, 220);
        }
    }

    int yStart = static_cast<int>(std::ceil(yRange.first));
    int yEnd = static_cast<int>(std::floor(yRange.second));
    for (int v = yStart; v <= yEnd; ++v) {
        int y = mapY(v, yRange);
        for (int x = left; x <= right; ++x) {
            image.setPixel(x, y, 220, 220, 220);
        }
    }
}

void PlotRenderer::drawAxisLabels(Image& image, const Range& xRange, const Range& yRange) const {
    int bottom = height_ - marginBottom_ + 8;
    int xStart = static_cast<int>(std::ceil(xRange.first));
    int xEnd = static_cast<int>(std::floor(xRange.second));
    if (xEnd - xStart < 1) {
        xStart = static_cast<int>(std::floor(xRange.first));
        xEnd = static_cast<int>(std::ceil(xRange.second));
    }
    for (int v = xStart; v <= xEnd; ++v) {
        int x = mapX(v, xRange);
        drawText(image, x - 5, bottom, std::to_string(v), 0, 0, 0);
    }

    int left = marginLeft_ - 35;
    int yStart = static_cast<int>(std::ceil(yRange.first));
    int yEnd = static_cast<int>(std::floor(yRange.second));
    if (yEnd - yStart < 1) {
        yStart = static_cast<int>(std::floor(yRange.first));
        yEnd = static_cast<int>(std::ceil(yRange.second));
    }
    for (int v = yStart; v <= yEnd; ++v) {
        int y = mapY(v, yRange);
        drawText(image, left, y - 3, std::to_string(v), 0, 0, 0);
    }
}

Image PlotRenderer::renderLinePlot(const std::vector<double>& xs, const std::vector<double>& ys) {
    Image image(width_, height_);
    image.clear(255, 255, 255);

    size_t count = std::min(xs.size(), ys.size());
    if (count == 0) {
        return image;
    }
    std::vector<double> xData(xs.begin(), xs.begin() + count);
    std::vector<double> yData(ys.begin(), ys.begin() + count);
    Range xRange = findRange(xData);
    Range yRange = findRange(yData);

    drawGrid(image, xRange, yRange);
    drawAxes(image, xRange, yRange);
    drawAxisLabels(image, xRange, yRange);

    int prevX = mapX(xData[0], xRange);
    int prevY = mapY(yData[0], yRange);
    for (size_t i = 1; i < count; ++i) {
        int x = mapX(xData[i], xRange);
        int y = mapY(yData[i], yRange);
        drawLine(image, prevX, prevY, x, y, 40, 90, 180);
        drawCircle(image, x, y, 3, 30, 60, 140);
        prevX = x;
        prevY = y;
    }
    drawCircle(image, mapX(xData[0], xRange), mapY(yData[0], yRange), 3, 30, 60, 140);
    return image;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double>& xs, const std::vector<double>& ys) {
    Image image(width_, height_);
    image.clear(255, 255, 255);

    size_t count = std::min(xs.size(), ys.size());
    if (count == 0) {
        return image;
    }
    std::vector<double> xData(xs.begin(), xs.begin() + count);
    std::vector<double> yData(ys.begin(), ys.begin() + count);
    Range xRange = findRange(xData);
    Range yRange = findRange(yData);

    drawGrid(image, xRange, yRange);
    drawAxes(image, xRange, yRange);
    drawAxisLabels(image, xRange, yRange);

    for (size_t i = 0; i < count; ++i) {
        int x = mapX(xData[i], xRange);
        int y = mapY(yData[i], yRange);
        drawCircle(image, x, y, 4, 200, 70, 40);
    }
    return image;
}

Image PlotRenderer::renderBarPlot(const std::vector<double>& xs, const std::vector<double>& ys) {
    Image image(width_, height_);
    image.clear(255, 255, 255);

    size_t count = std::min(xs.size(), ys.size());
    if (count == 0) {
        return image;
    }
    std::vector<double> xData(xs.begin(), xs.begin() + count);
    std::vector<double> yData(ys.begin(), ys.begin() + count);
    Range xRange = findRange(xData);
    Range yRange = findRange(yData);
    yRange.first = std::min(0.0, yRange.first);
    yRange.second = std::max(0.0, yRange.second);

    drawGrid(image, xRange, yRange);
    drawAxes(image, xRange, yRange);
    drawAxisLabels(image, xRange, yRange);

    int plotWidth = width_ - marginLeft_ - marginRight_;
    double barWidth = static_cast<double>(plotWidth) / static_cast<double>(count);
    for (size_t i = 0; i < count; ++i) {
        int xCenter = mapX(xData[i], xRange);
        int half = static_cast<int>(barWidth * 0.35);
        int xStart = xCenter - half;
        int xEnd = xCenter + half;
        int yBase = mapY(0.0, yRange);
        int yTop = mapY(yData[i], yRange);
        if (yTop < yBase) {
            for (int x = xStart; x <= xEnd; ++x) {
                for (int y = yTop; y <= yBase; ++y) {
                    image.setPixel(x, y, 60, 160, 80);
                }
            }
        } else {
            for (int x = xStart; x <= xEnd; ++x) {
                for (int y = yBase; y <= yTop; ++y) {
                    image.setPixel(x, y, 200, 80, 60);
                }
            }
        }
    }
    return image;
}
