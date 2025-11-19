#include "PlotRenderer.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace {
constexpr unsigned char kBackground = 250;
constexpr unsigned char kAxisColor = 30;
constexpr unsigned char kGridColor = 200;
constexpr unsigned char kLineColorR = 40;
constexpr unsigned char kLineColorG = 120;
constexpr unsigned char kLineColorB = 240;
constexpr unsigned char kScatterColorR = 200;
constexpr unsigned char kScatterColorG = 60;
constexpr unsigned char kScatterColorB = 80;
constexpr unsigned char kBarColorR = 90;
constexpr unsigned char kBarColorG = 180;
constexpr unsigned char kBarColorB = 90;

int clampInt(int value, int minValue, int maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

const char *patternForChar(char c) {
    switch (c) {
    case '0': return "111101101101111";
    case '1': return "010110010010111";
    case '2': return "111001111100111";
    case '3': return "111001111001111";
    case '4': return "101101111001001";
    case '5': return "111100111001111";
    case '6': return "111100111101111";
    case '7': return "111001001001001";
    case '8': return "111101111101111";
    case '9': return "111101111001111";
    case '-': return "000000111000000";
    case '.': return "000000000000010";
    default: return nullptr;
    }
}
}

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width),
      height_(height),
      marginLeft_(70),
      marginRight_(40),
      marginTop_(40),
      marginBottom_(70) {}

PlotRenderer::Range PlotRenderer::findMinMax(const std::vector<double> &values) const {
    if (values.empty()) {
        return {0.0, 1.0};
    }
    auto minMax = std::minmax_element(values.begin(), values.end());
    double minVal = *minMax.first;
    double maxVal = *minMax.second;
    if (std::abs(maxVal - minVal) < 1e-9) {
        minVal -= 1.0;
        maxVal += 1.0;
    }
    return {minVal, maxVal};
}

int PlotRenderer::mapX(double value, const Range &range) const {
    const double plotWidth = static_cast<double>(width_ - marginLeft_ - marginRight_);
    const double ratio = (value - range.first) / (range.second - range.first);
    const int x = marginLeft_ + static_cast<int>(std::round(ratio * plotWidth));
    return clampInt(x, marginLeft_, width_ - marginRight_);
}

int PlotRenderer::mapY(double value, const Range &range) const {
    const double plotHeight = static_cast<double>(height_ - marginTop_ - marginBottom_);
    const double ratio = (value - range.first) / (range.second - range.first);
    const double y = static_cast<double>(height_ - marginBottom_) - ratio * plotHeight;
    return clampInt(static_cast<int>(std::round(y)), marginTop_, height_ - marginBottom_);
}

void PlotRenderer::drawAxes(Image &image) const {
    const int left = marginLeft_;
    const int right = width_ - marginRight_;
    const int top = marginTop_;
    const int bottom = height_ - marginBottom_;

    drawLine(image, left, top, left, bottom, kAxisColor, kAxisColor, kAxisColor);
    drawLine(image, left, bottom, right, bottom, kAxisColor, kAxisColor, kAxisColor);
}

void PlotRenderer::drawGrid(Image &image, const Range &xRange, const Range &yRange) const {
    const int left = marginLeft_;
    const int right = width_ - marginRight_;
    const int top = marginTop_;
    const int bottom = height_ - marginBottom_;

    const int startX = static_cast<int>(std::ceil(xRange.first));
    const int endX = static_cast<int>(std::floor(xRange.second));
    for (int value = startX; value <= endX; ++value) {
        const int x = mapX(static_cast<double>(value), xRange);
        drawLine(image, x, top, x, bottom, kGridColor, kGridColor, kGridColor);
    }

    const int startY = static_cast<int>(std::ceil(yRange.first));
    const int endY = static_cast<int>(std::floor(yRange.second));
    for (int value = startY; value <= endY; ++value) {
        const int y = mapY(static_cast<double>(value), yRange);
        drawLine(image, left, y, right, y, kGridColor, kGridColor, kGridColor);
    }
}

void PlotRenderer::drawAxisLabels(Image &image, const Range &xRange, const Range &yRange) const {
    const int bottom = height_ - marginBottom_ + 10;
    const int left = marginLeft_ - 40;

    const int startX = static_cast<int>(std::ceil(xRange.first));
    const int endX = static_cast<int>(std::floor(xRange.second));
    for (int value = startX; value <= endX; ++value) {
        const int x = mapX(static_cast<double>(value), xRange) - 6;
        drawText(image, x, bottom, formatNumber(static_cast<double>(value)),
                 kAxisColor, kAxisColor, kAxisColor);
    }

    const int startY = static_cast<int>(std::ceil(yRange.first));
    const int endY = static_cast<int>(std::floor(yRange.second));
    for (int value = startY; value <= endY; ++value) {
        const int y = mapY(static_cast<double>(value), yRange) - 8;
        drawText(image, left, y, formatNumber(static_cast<double>(value)),
                 kAxisColor, kAxisColor, kAxisColor);
    }
}

void PlotRenderer::drawLine(Image &image, int x0, int y0, int x1, int y1,
                            unsigned char r, unsigned char g, unsigned char b) const {
    const bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = std::abs(y1 - y0);
    int error = dx / 2;
    int yStep = (y0 < y1) ? 1 : -1;
    int y = y0;

    for (int x = x0; x <= x1; ++x) {
        if (steep) {
            image.setPixel(y, x, r, g, b);
        } else {
            image.setPixel(x, y, r, g, b);
        }
        error -= dy;
        if (error < 0) {
            y += yStep;
            error += dx;
        }
    }
}

void PlotRenderer::drawCircle(Image &image, int cx, int cy, int radius,
                              unsigned char r, unsigned char g, unsigned char b) const {
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                image.setPixel(cx + x, cy + y, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image &image, int cx, int cy, int half,
                              unsigned char r, unsigned char g, unsigned char b) const {
    for (int y = -half; y <= half; ++y) {
        for (int x = -half; x <= half; ++x) {
            image.setPixel(cx + x, cy + y, r, g, b);
        }
    }
}

void PlotRenderer::drawDigit(Image &image, int x, int y, char digit,
                             unsigned char r, unsigned char g, unsigned char b) const {
    const char *pattern = patternForChar(digit);
    if (!pattern) {
        return;
    }
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            const int index = row * 3 + col;
            if (pattern[index] == '1') {
                image.setPixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawText(Image &image, int x, int y, const std::string &text,
                            unsigned char r, unsigned char g, unsigned char b) const {
    int cursor = x;
    for (char c : text) {
        if (c == ' ') {
            cursor += 4;
            continue;
        }
        drawDigit(image, cursor, y, c, r, g, b);
        cursor += 4;
    }
}

std::string PlotRenderer::formatNumber(double value) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << value;
    std::string result = ss.str();
    while (result.size() > 1 && result.back() == '0') {
        result.pop_back();
    }
    if (!result.empty() && result.back() == '.') {
        result.pop_back();
    }
    return result;
}

void PlotRenderer::renderLineSegments(Image &image, const std::vector<int> &pointsX,
                                      const std::vector<int> &pointsY) const {
    for (size_t i = 1; i < pointsX.size(); ++i) {
        drawLine(image, pointsX[i - 1], pointsY[i - 1], pointsX[i], pointsY[i],
                 kLineColorR, kLineColorG, kLineColorB);
    }
}

Image PlotRenderer::renderLinePlot(const std::vector<double> &xs, const std::vector<double> &ys) {
    Image image(width_, height_);
    image.clear(kBackground, kBackground, kBackground);

    if (xs.empty() || ys.empty() || xs.size() != ys.size()) {
        return image;
    }

    const Range xRange = findMinMax(xs);
    const Range yRange = findMinMax(ys);

    drawGrid(image, xRange, yRange);
    drawAxes(image);
    drawAxisLabels(image, xRange, yRange);

    std::vector<int> pointsX;
    std::vector<int> pointsY;
    pointsX.reserve(xs.size());
    pointsY.reserve(xs.size());

    for (size_t i = 0; i < xs.size(); ++i) {
        pointsX.push_back(mapX(xs[i], xRange));
        pointsY.push_back(mapY(ys[i], yRange));
    }

    renderLineSegments(image, pointsX, pointsY);

    for (size_t i = 0; i < pointsX.size(); ++i) {
        drawCircle(image, pointsX[i], pointsY[i], 3, kScatterColorR, kScatterColorG, kScatterColorB);
    }

    return image;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double> &xs, const std::vector<double> &ys) {
    Image image(width_, height_);
    image.clear(kBackground, kBackground, kBackground);

    if (xs.empty() || ys.empty() || xs.size() != ys.size()) {
        return image;
    }

    const Range xRange = findMinMax(xs);
    const Range yRange = findMinMax(ys);

    drawGrid(image, xRange, yRange);
    drawAxes(image);
    drawAxisLabels(image, xRange, yRange);

    for (size_t i = 0; i < xs.size(); ++i) {
        const int x = mapX(xs[i], xRange);
        const int y = mapY(ys[i], yRange);
        drawCircle(image, x, y, 4, kScatterColorR, kScatterColorG, kScatterColorB);
    }

    return image;
}

Image PlotRenderer::renderBarPlot(const std::vector<double> &xs, const std::vector<double> &ys) {
    Image image(width_, height_);
    image.clear(kBackground, kBackground, kBackground);

    if (xs.empty() || ys.empty() || xs.size() != ys.size()) {
        return image;
    }

    const Range xRange = findMinMax(xs);
    Range yRange = findMinMax(ys);
    yRange.first = std::min(0.0, yRange.first);
    yRange.second = std::max(0.0, yRange.second);

    drawGrid(image, xRange, yRange);
    drawAxes(image);
    drawAxisLabels(image, xRange, yRange);

    const double plotWidth = static_cast<double>(width_ - marginLeft_ - marginRight_);
    const double barTotalWidth = plotWidth / static_cast<double>(std::max<size_t>(1, xs.size()));
    const int halfWidth = static_cast<int>(std::max(2.0, std::floor(barTotalWidth * 0.4)));
    const int zeroY = mapY(0.0, yRange);

    for (size_t i = 0; i < xs.size(); ++i) {
        const int centerX = mapX(xs[i], xRange);
        const int yValue = mapY(ys[i], yRange);
        const int top = std::min(yValue, zeroY);
        const int bottom = std::max(yValue, zeroY);
        for (int x = centerX - halfWidth; x <= centerX + halfWidth; ++x) {
            for (int y = top; y <= bottom; ++y) {
                image.setPixel(x, y, kBarColorR, kBarColorG, kBarColorB);
            }
        }
        drawSquare(image, centerX, yValue, 2, kAxisColor, kAxisColor, kAxisColor);
    }

    return image;
}
