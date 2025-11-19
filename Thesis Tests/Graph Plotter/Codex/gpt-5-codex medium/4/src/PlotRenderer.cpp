#include "PlotRenderer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
constexpr unsigned char COLOR_AXIS = 30;
constexpr unsigned char COLOR_GRID = 200;
constexpr unsigned char COLOR_TEXT = 40;
constexpr unsigned char COLOR_LINE_R = 44;
constexpr unsigned char COLOR_LINE_G = 85;
constexpr unsigned char COLOR_LINE_B = 224;
constexpr unsigned char COLOR_SCATTER_R = 204;
constexpr unsigned char COLOR_SCATTER_G = 68;
constexpr unsigned char COLOR_SCATTER_B = 85;
constexpr unsigned char COLOR_BAR_R = 70;
constexpr unsigned char COLOR_BAR_G = 170;
constexpr unsigned char COLOR_BAR_B = 120;
}

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width), height_(height) {
    if (plotWidth() <= 0 || plotHeight() <= 0) {
        throw std::runtime_error("Invalid canvas dimensions");
    }
}

int PlotRenderer::plotWidth() const {
    return std::max(1, width_ - marginLeft_ - marginRight_);
}

int PlotRenderer::plotHeight() const {
    return std::max(1, height_ - marginTop_ - marginBottom_);
}

PlotRenderer::Range PlotRenderer::computeRange(const std::vector<double> &values) const {
    if (values.empty()) {
        throw std::runtime_error("No values provided");
    }
    double minVal = values.front();
    double maxVal = values.front();
    for (double v : values) {
        minVal = std::min(minVal, v);
        maxVal = std::max(maxVal, v);
    }
    if (minVal == maxVal) {
        minVal -= 1.0;
        maxVal += 1.0;
    }
    return {minVal, maxVal};
}

int PlotRenderer::mapX(double value, const Range &range) const {
    double normalized = (value - range.min) / (range.max - range.min);
    normalized = std::clamp(normalized, 0.0, 1.0);
    return marginLeft_ + static_cast<int>(std::round(normalized * (plotWidth() - 1)));
}

int PlotRenderer::mapY(double value, const Range &range) const {
    double normalized = (value - range.min) / (range.max - range.min);
    normalized = std::clamp(normalized, 0.0, 1.0);
    int plotArea = plotHeight() - 1;
    return height_ - marginBottom_ - static_cast<int>(std::round(normalized * plotArea));
}

void PlotRenderer::drawAxes(Image &image) const {
    int xAxisY = height_ - marginBottom_;
    int yAxisX = marginLeft_;
    for (int x = marginLeft_; x < width_ - marginRight_; ++x) {
        image.setPixel(x, xAxisY, COLOR_AXIS, COLOR_AXIS, COLOR_AXIS);
    }
    for (int y = marginTop_; y < height_ - marginBottom_; ++y) {
        image.setPixel(yAxisX, y, COLOR_AXIS, COLOR_AXIS, COLOR_AXIS);
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
    int ystep = (y0 < y1) ? 1 : -1;
    int y = y0;

    for (int x = x0; x <= x1; ++x) {
        if (steep) {
            image.setPixel(y, x, r, g, b);
        } else {
            image.setPixel(x, y, r, g, b);
        }
        error -= dy;
        if (error < 0) {
            y += ystep;
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

void PlotRenderer::drawSquare(Image &image, int cx, int cy, int size,
                              unsigned char r, unsigned char g, unsigned char b) const {
    int half = size / 2;
    for (int y = -half; y <= half; ++y) {
        for (int x = -half; x <= half; ++x) {
            image.setPixel(cx + x, cy + y, r, g, b);
        }
    }
}

void PlotRenderer::drawGrid(Image &image, const Range &xRange, const Range &yRange) const {
    int plotTop = marginTop_;
    int plotBottom = height_ - marginBottom_;
    int plotLeft = marginLeft_;
    int plotRight = width_ - marginRight_;

    auto drawVertical = [&](double value) {
        int x = mapX(value, xRange);
        for (int y = plotTop; y <= plotBottom; ++y) {
            image.setPixel(x, y, COLOR_GRID, COLOR_GRID, COLOR_GRID);
        }
    };
    auto drawHorizontal = [&](double value) {
        int y = mapY(value, yRange);
        for (int x = plotLeft; x <= plotRight; ++x) {
            image.setPixel(x, y, COLOR_GRID, COLOR_GRID, COLOR_GRID);
        }
    };

    int xStart = static_cast<int>(std::ceil(xRange.min));
    int xEnd = static_cast<int>(std::floor(xRange.max));
    if (xStart > xEnd) {
        drawVertical(xRange.min);
    } else {
        for (int v = xStart; v <= xEnd; ++v) {
            drawVertical(v);
        }
    }

    int yStart = static_cast<int>(std::ceil(yRange.min));
    int yEnd = static_cast<int>(std::floor(yRange.max));
    if (yStart > yEnd) {
        drawHorizontal(yRange.min);
    } else {
        for (int v = yStart; v <= yEnd; ++v) {
            drawHorizontal(v);
        }
    }
}

void PlotRenderer::drawDigit(Image &image, int x, int y, char digit,
                             unsigned char r, unsigned char g, unsigned char b) const {
    static const char *patterns[] = {
        "111101101101111", // 0
        "010110010010111", // 1
        "111001111100111", // 2
        "111001111001111", // 3
        "101101111001001", // 4
        "111100111001111", // 5
        "111100111101111", // 6
        "111001001001001", // 7
        "111101111101111", // 8
        "111101111001111", // 9
        "000000000000010", // .
        "000000111000000"  // -
    };

    int index;
    if (digit >= '0' && digit <= '9') {
        index = digit - '0';
    } else if (digit == '.') {
        index = 10;
    } else if (digit == '-') {
        index = 11;
    } else {
        return;
    }

    const char *pattern = patterns[index];
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            char bit = pattern[row * 3 + col];
            if (bit == '1') {
                image.setPixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawText(Image &image, int x, int y, const std::string &text,
                            unsigned char r, unsigned char g, unsigned char b) const {
    int cursor = x;
    for (char ch : text) {
        drawDigit(image, cursor, y, ch, r, g, b);
        cursor += 4;
    }
}

void PlotRenderer::drawAxisLabels(Image &image, const Range &xRange, const Range &yRange) const {
    int baselineY = height_ - marginBottom_ + 15;
    int labelStart = marginLeft_ - 40;

    int xStart = static_cast<int>(std::ceil(xRange.min));
    int xEnd = static_cast<int>(std::floor(xRange.max));
    if (xStart > xEnd) {
        int px = mapX(xRange.min, xRange);
        drawText(image, px - 6, baselineY, std::to_string(static_cast<int>(xRange.min)), COLOR_TEXT, COLOR_TEXT, COLOR_TEXT);
    } else {
        for (int v = xStart; v <= xEnd; ++v) {
            int px = mapX(static_cast<double>(v), xRange) - 6;
            drawText(image, px, baselineY, std::to_string(v), COLOR_TEXT, COLOR_TEXT, COLOR_TEXT);
        }
    }

    int yStart = static_cast<int>(std::ceil(yRange.min));
    int yEnd = static_cast<int>(std::floor(yRange.max));
    if (yStart > yEnd) {
        int py = mapY(yRange.min, yRange);
        drawText(image, labelStart, py - 2, std::to_string(static_cast<int>(yRange.min)), COLOR_TEXT, COLOR_TEXT, COLOR_TEXT);
    } else {
        for (int v = yStart; v <= yEnd; ++v) {
            int py = mapY(static_cast<double>(v), yRange) - 2;
            drawText(image, labelStart, py, std::to_string(v), COLOR_TEXT, COLOR_TEXT, COLOR_TEXT);
        }
    }
}

void PlotRenderer::commonDecorations(Image &image, const Range &xRange, const Range &yRange) const {
    image.clear(255, 255, 255);
    drawGrid(image, xRange, yRange);
    drawAxes(image);
    drawAxisLabels(image, xRange, yRange);
}

Image PlotRenderer::renderLinePlot(const std::vector<double> &xData,
                                   const std::vector<double> &yData) const {
    if (xData.size() != yData.size()) {
        throw std::runtime_error("X and Y data size mismatch");
    }
    Image image(width_, height_);
    auto xRange = computeRange(xData);
    auto yRange = computeRange(yData);
    commonDecorations(image, xRange, yRange);

    for (size_t i = 1; i < xData.size(); ++i) {
        int x0 = mapX(xData[i - 1], xRange);
        int y0 = mapY(yData[i - 1], yRange);
        int x1 = mapX(xData[i], xRange);
        int y1 = mapY(yData[i], yRange);
        drawLine(image, x0, y0, x1, y1, COLOR_LINE_R, COLOR_LINE_G, COLOR_LINE_B);
    }

    return image;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double> &xData,
                                      const std::vector<double> &yData) const {
    if (xData.size() != yData.size()) {
        throw std::runtime_error("X and Y data size mismatch");
    }
    Image image(width_, height_);
    auto xRange = computeRange(xData);
    auto yRange = computeRange(yData);
    commonDecorations(image, xRange, yRange);

    for (size_t i = 0; i < xData.size(); ++i) {
        int px = mapX(xData[i], xRange);
        int py = mapY(yData[i], yRange);
        drawCircle(image, px, py, 3, COLOR_SCATTER_R, COLOR_SCATTER_G, COLOR_SCATTER_B);
    }
    return image;
}

Image PlotRenderer::renderBarPlot(const std::vector<double> &xData,
                                  const std::vector<double> &yData) const {
    if (xData.size() != yData.size()) {
        throw std::runtime_error("X and Y data size mismatch");
    }
    Image image(width_, height_);
    auto xRange = computeRange(xData);
    auto yRange = computeRange(yData);
    commonDecorations(image, xRange, yRange);

    int availableWidth = plotWidth();
    int spacing = std::max(1, availableWidth / static_cast<int>(xData.size() * 2));
    int halfBar = std::max(2, spacing);

    double zeroReference;
    if (yRange.min > 0) {
        zeroReference = yRange.min;
    } else if (yRange.max < 0) {
        zeroReference = yRange.max;
    } else {
        zeroReference = 0.0;
    }
    int baseline = mapY(zeroReference, yRange);

    for (size_t i = 0; i < xData.size(); ++i) {
        int px = mapX(xData[i], xRange);
        int py = mapY(yData[i], yRange);
        int yStart = std::min(py, baseline);
        int yEnd = std::max(py, baseline);
        for (int x = px - halfBar; x <= px + halfBar; ++x) {
            for (int y = yStart; y <= yEnd; ++y) {
                image.setPixel(x, y, COLOR_BAR_R, COLOR_BAR_G, COLOR_BAR_B);
            }
        }
    }

    return image;
}
