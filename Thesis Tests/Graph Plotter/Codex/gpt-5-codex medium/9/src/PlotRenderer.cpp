#include "PlotRenderer.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace {
const unsigned char COLOR_BACKGROUND = 255;
const unsigned char COLOR_GRID = 220;
const unsigned char COLOR_AXIS = 40;
const unsigned char COLOR_TEXT = 30;
const unsigned char COLOR_LINE_R = 30;
const unsigned char COLOR_LINE_G = 99;
const unsigned char COLOR_LINE_B = 200;
}

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width),
      height_(height),
      marginLeft_(80),
      marginRight_(40),
      marginTop_(40),
      marginBottom_(80) {}

PlotRenderer::Range PlotRenderer::calculateRange(const std::vector<double>& values) const {
    if (values.empty()) {
        return {0.0, 1.0};
    }
    double minVal = values.front();
    double maxVal = values.front();
    for (double v : values) {
        minVal = std::min(minVal, v);
        maxVal = std::max(maxVal, v);
    }
    if (std::abs(maxVal - minVal) < 1e-9) {
        maxVal = minVal + 1.0;
    }
    return {minVal, maxVal};
}

int PlotRenderer::mapX(double x, Range xRange) const {
    const int plotWidth = width_ - marginLeft_ - marginRight_;
    if (plotWidth <= 0) {
        return marginLeft_;
    }
    const double span = (xRange.max - xRange.min);
    const double norm = (span == 0.0) ? 0.0 : (x - xRange.min) / span;
    const double clamped = std::min(std::max(norm, 0.0), 1.0);
    return marginLeft_ + static_cast<int>(clamped * plotWidth);
}

int PlotRenderer::mapY(double y, Range yRange) const {
    const int plotHeight = height_ - marginTop_ - marginBottom_;
    if (plotHeight <= 0) {
        return height_ - marginBottom_;
    }
    const double span = (yRange.max - yRange.min);
    const double norm = (span == 0.0) ? 0.0 : (y - yRange.min) / span;
    const double clamped = std::min(std::max(norm, 0.0), 1.0);
    return height_ - marginBottom_ - static_cast<int>(clamped * plotHeight);
}

void PlotRenderer::drawLine(Image& image, int x0, int y0, int x1, int y1,
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
    const int dx = x1 - x0;
    const int dy = std::abs(y1 - y0);
    int error = dx / 2;
    const int yStep = (y0 < y1) ? 1 : -1;
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
    const int half = size / 2;
    for (int y = -half; y <= half; ++y) {
        for (int x = -half; x <= half; ++x) {
            image.setPixel(cx + x, cy + y, r, g, b);
        }
    }
}

void PlotRenderer::drawDigit(Image& image, int x, int y, char digit,
                             unsigned char r, unsigned char g, unsigned char b) const {
    static const int font[11][15] = {
        {1,1,1,1,0,1,1,0,1,1,1,1,1,0,1}, // 0
        {0,1,0,1,1,0,0,1,0,0,1,0,1,1,1}, // 1
        {1,1,1,0,0,1,1,1,1,1,0,0,1,1,1}, // 2
        {1,1,1,0,0,1,1,1,1,0,0,1,1,1,1}, // 3
        {1,0,1,1,0,1,1,1,1,0,0,1,0,0,1}, // 4
        {1,1,1,1,0,0,1,1,1,0,0,1,1,1,1}, // 5
        {1,1,1,1,0,0,1,1,1,1,0,1,1,1,1}, // 6
        {1,1,1,0,0,1,0,0,1,0,0,1,0,0,1}, // 7
        {1,1,1,1,0,1,1,1,1,1,0,1,1,1,1}, // 8
        {1,1,1,1,0,1,1,1,1,0,0,1,1,1,1}, // 9
        {0,0,0,0,0,0,1,1,1,0,0,0,0,0,0}  // -
    };
    if (digit == ' ') {
        return;
    }
    if (digit == '.') {
        image.setPixel(x + 1, y + 4, r, g, b);
        return;
    }
    if (digit == '-') {
        for (int i = 0; i < 3; ++i) {
            image.setPixel(x + i, y + 2, r, g, b);
        }
        return;
    }
    if (digit < '0' || digit > '9') {
        return;
    }
    const int* bitmap = font[digit - '0'];
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (bitmap[row * 3 + col]) {
                image.setPixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawText(Image& image, int x, int y, const std::string& text,
                            unsigned char r, unsigned char g, unsigned char b) const {
    int cursor = x;
    for (char c : text) {
        drawDigit(image, cursor, y, c, r, g, b);
        cursor += 4;
    }
}

void PlotRenderer::drawGrid(Image& image, Range xRange, Range yRange) const {
    const int left = marginLeft_;
    const int right = width_ - marginRight_;
    const int top = marginTop_;
    const int bottom = height_ - marginBottom_;

    const int xStart = static_cast<int>(std::ceil(xRange.min));
    const int xEnd = static_cast<int>(std::floor(xRange.max));
    for (int value = xStart; value <= xEnd; ++value) {
        const int x = mapX(static_cast<double>(value), xRange);
        for (int y = top; y <= bottom; ++y) {
            image.setPixel(x, y, COLOR_GRID, COLOR_GRID, COLOR_GRID);
        }
    }

    const int yStart = static_cast<int>(std::ceil(yRange.min));
    const int yEnd = static_cast<int>(std::floor(yRange.max));
    for (int value = yStart; value <= yEnd; ++value) {
        const int y = mapY(static_cast<double>(value), yRange);
        for (int x = left; x <= right; ++x) {
            image.setPixel(x, y, COLOR_GRID, COLOR_GRID, COLOR_GRID);
        }
    }
}

void PlotRenderer::drawAxisLabels(Image& image, Range xRange, Range yRange) const {
    const int yLabelY = height_ - marginBottom_ + 10;
    const int xStart = static_cast<int>(std::ceil(xRange.min));
    const int xEnd = static_cast<int>(std::floor(xRange.max));
    for (int value = xStart; value <= xEnd; ++value) {
        const int x = mapX(static_cast<double>(value), xRange) - 4;
        drawText(image, x, yLabelY, std::to_string(value), COLOR_TEXT, COLOR_TEXT, COLOR_TEXT);
    }

    const int xLabelX = marginLeft_ - 30;
    const int yStart = static_cast<int>(std::ceil(yRange.min));
    const int yEnd = static_cast<int>(std::floor(yRange.max));
    for (int value = yStart; value <= yEnd; ++value) {
        const int y = mapY(static_cast<double>(value), yRange) - 2;
        drawText(image, xLabelX, y, std::to_string(value), COLOR_TEXT, COLOR_TEXT, COLOR_TEXT);
    }
}

void PlotRenderer::drawAxes(Image& image, Range, Range) const {
    const int left = marginLeft_;
    const int right = width_ - marginRight_;
    const int top = marginTop_;
    const int bottom = height_ - marginBottom_;
    drawLine(image, left, top, left, bottom, COLOR_AXIS, COLOR_AXIS, COLOR_AXIS);
    drawLine(image, left, bottom, right, bottom, COLOR_AXIS, COLOR_AXIS, COLOR_AXIS);
}

Image PlotRenderer::renderLinePlot(const std::vector<double>& xs,
                                   const std::vector<double>& ys,
                                   Range xRange,
                                   Range yRange) const {
    Image image(width_, height_);
    image.clear(COLOR_BACKGROUND, COLOR_BACKGROUND, COLOR_BACKGROUND);
    drawGrid(image, xRange, yRange);
    drawAxes(image, xRange, yRange);
    drawAxisLabels(image, xRange, yRange);

    for (size_t i = 1; i < xs.size(); ++i) {
        const int x0 = mapX(xs[i - 1], xRange);
        const int y0 = mapY(ys[i - 1], yRange);
        const int x1 = mapX(xs[i], xRange);
        const int y1 = mapY(ys[i], yRange);
        drawLine(image, x0, y0, x1, y1, COLOR_LINE_R, COLOR_LINE_G, COLOR_LINE_B);
    }
    for (size_t i = 0; i < xs.size(); ++i) {
        const int x = mapX(xs[i], xRange);
        const int y = mapY(ys[i], yRange);
        drawCircle(image, x, y, 3, COLOR_LINE_R, COLOR_LINE_G, COLOR_LINE_B);
    }
    return image;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double>& xs,
                                      const std::vector<double>& ys,
                                      Range xRange,
                                      Range yRange) const {
    Image image(width_, height_);
    image.clear(COLOR_BACKGROUND, COLOR_BACKGROUND, COLOR_BACKGROUND);
    drawGrid(image, xRange, yRange);
    drawAxes(image, xRange, yRange);
    drawAxisLabels(image, xRange, yRange);

    for (size_t i = 0; i < xs.size(); ++i) {
        const int x = mapX(xs[i], xRange);
        const int y = mapY(ys[i], yRange);
        drawSquare(image, x, y, 7, COLOR_LINE_R, COLOR_LINE_G, COLOR_LINE_B);
    }
    return image;
}

Image PlotRenderer::renderBarPlot(const std::vector<double>& xs,
                                  const std::vector<double>& ys,
                                  Range xRange,
                                  Range yRange) const {
    Image image(width_, height_);
    image.clear(COLOR_BACKGROUND, COLOR_BACKGROUND, COLOR_BACKGROUND);
    drawGrid(image, xRange, yRange);
    drawAxes(image, xRange, yRange);
    drawAxisLabels(image, xRange, yRange);

    const int plotWidth = width_ - marginLeft_ - marginRight_;
    const int baseY = mapY(0.0, yRange);
    const int barWidth = std::max(4, plotWidth / static_cast<int>(std::max<size_t>(1, xs.size())));

    for (size_t i = 0; i < xs.size(); ++i) {
        const int centerX = mapX(xs[i], xRange);
        const int halfWidth = std::max(2, barWidth / 2 - 2);
        const int xStart = centerX - halfWidth;
        const int xEnd = centerX + halfWidth;
        const int yValue = mapY(ys[i], yRange);
        const int topY = std::min(baseY, yValue);
        const int bottomY = std::max(baseY, yValue);
        for (int x = xStart; x <= xEnd; ++x) {
            for (int y = topY; y <= bottomY; ++y) {
                image.setPixel(x, y, COLOR_LINE_R, COLOR_LINE_G, COLOR_LINE_B);
            }
        }
    }
    return image;
}

Image PlotRenderer::render(const std::vector<double>& xsInput,
                           const std::vector<double>& ysInput,
                           const std::string& plotType) const {
    std::vector<double> xs = xsInput;
    std::vector<double> ys = ysInput;
    if (xs.empty() && !ys.empty()) {
        xs.resize(ys.size());
        for (size_t i = 0; i < ys.size(); ++i) {
            xs[i] = static_cast<double>(i);
        }
    }
    if (ys.empty() && !xs.empty()) {
        ys.resize(xs.size(), 0.0);
    }
    if (xs.size() != ys.size()) {
        const size_t minSize = std::min(xs.size(), ys.size());
        xs.resize(minSize);
        ys.resize(minSize);
    }
    if (xs.empty()) {
        xs = {0.0};
        ys = {0.0};
    }

    const Range xRange = calculateRange(xs);
    const Range yRange = calculateRange(ys);

    if (plotType == "scatter") {
        return renderScatterPlot(xs, ys, xRange, yRange);
    }
    if (plotType == "bar") {
        return renderBarPlot(xs, ys, xRange, yRange);
    }
    return renderLinePlot(xs, ys, xRange, yRange);
}
