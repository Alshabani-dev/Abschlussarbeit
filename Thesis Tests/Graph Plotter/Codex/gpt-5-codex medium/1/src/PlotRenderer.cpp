#include "PlotRenderer.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <stdexcept>

namespace {
constexpr int kMarginLeft = 60;
constexpr int kMarginRight = 20;
constexpr int kMarginTop = 20;
constexpr int kMarginBottom = 60;

constexpr unsigned char kBackground = 255;
constexpr unsigned char kForeground = 30;
constexpr unsigned char kGrid = 200;
constexpr unsigned char kAxis = 0;
constexpr unsigned char kLineColor = 30;
constexpr unsigned char kScatterColor = 200;
constexpr unsigned char kBarColor = 30;
} // namespace

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width), height_(height) {}

Image PlotRenderer::render(const PlotRequest& request) {
    if (request.xs.empty() || request.ys.empty() || request.xs.size() != request.ys.size()) {
        throw std::runtime_error("Invalid input data for plot");
    }

    Image image(width_, height_);
    image.clear(kBackground, kBackground, kBackground);

    Range xRange = findRange(request.xs);
    Range yRange = findRange(request.ys);

    drawGrid(image, xRange, yRange);
    drawAxes(image, xRange, yRange);
    drawLabels(image, xRange, yRange);

    switch (request.type) {
    case PlotType::Line:
        drawLinePlot(image, request, xRange, yRange);
        break;
    case PlotType::Scatter:
        drawScatterPlot(image, request, xRange, yRange);
        break;
    case PlotType::Bar:
        drawBarPlot(image, request, xRange, yRange);
        break;
    }

    return image;
}

PlotRenderer::Range PlotRenderer::findRange(const std::vector<double>& values) const {
    Range range{values.front(), values.front()};
    for (double value : values) {
        if (value < range.min) {
            range.min = value;
        }
        if (value > range.max) {
            range.max = value;
        }
    }
    if (range.min == range.max) {
        double padding = std::abs(range.min) * 0.1 + 1.0;
        range.min -= padding;
        range.max += padding;
    }
    return range;
}

int PlotRenderer::mapX(double value, const Range& range) const {
    double ratio = (value - range.min) / (range.max - range.min);
    int plotWidth = width_ - kMarginLeft - kMarginRight;
    int x = static_cast<int>(kMarginLeft + ratio * plotWidth + 0.5);
    return std::min(std::max(x, kMarginLeft), width_ - kMarginRight - 1);
}

int PlotRenderer::mapY(double value, const Range& range) const {
    double ratio = (value - range.min) / (range.max - range.min);
    int plotHeight = height_ - kMarginTop - kMarginBottom;
    int y = static_cast<int>(height_ - kMarginBottom - ratio * plotHeight + 0.5);
    return std::min(std::max(y, kMarginTop), height_ - kMarginBottom - 1);
}

void PlotRenderer::drawAxes(Image& image, const Range& xRange, const Range& yRange) const {
    (void)xRange;
    (void)yRange;
    int left = kMarginLeft;
    int right = width_ - kMarginRight;
    int top = kMarginTop;
    int bottom = height_ - kMarginBottom;

    drawLine(image, left, top, left, bottom, kAxis, kAxis, kAxis);
    drawLine(image, left, bottom, right, bottom, kAxis, kAxis, kAxis);
}

void PlotRenderer::drawGrid(Image& image, const Range& xRange, const Range& yRange) const {
    int left = kMarginLeft;
    int right = width_ - kMarginRight;
    int top = kMarginTop;
    int bottom = height_ - kMarginBottom;

    for (int i = static_cast<int>(std::ceil(xRange.min)); i <= static_cast<int>(std::floor(xRange.max)); ++i) {
        int x = mapX(static_cast<double>(i), xRange);
        drawLine(image, x, top, x, bottom, kGrid, kGrid, kGrid);
    }

    for (int i = static_cast<int>(std::ceil(yRange.min)); i <= static_cast<int>(std::floor(yRange.max)); ++i) {
        int y = mapY(static_cast<double>(i), yRange);
        drawLine(image, left, y, right, y, kGrid, kGrid, kGrid);
    }
}

void PlotRenderer::drawLabels(Image& image, const Range& xRange, const Range& yRange) const {
    for (int i = static_cast<int>(std::ceil(xRange.min)); i <= static_cast<int>(std::floor(xRange.max)); ++i) {
        int x = mapX(static_cast<double>(i), xRange);
        drawText(image, x - 6, height_ - kMarginBottom + 10, std::to_string(i), kAxis, kAxis, kAxis);
    }

    for (int i = static_cast<int>(std::ceil(yRange.min)); i <= static_cast<int>(std::floor(yRange.max)); ++i) {
        int y = mapY(static_cast<double>(i), yRange);
        drawText(image, 5, y - 4, std::to_string(i), kAxis, kAxis, kAxis);
    }
}

void PlotRenderer::drawLinePlot(Image& image, const PlotRequest& request, const Range& xRange, const Range& yRange) const {
    for (size_t i = 1; i < request.xs.size(); ++i) {
        int x0 = mapX(request.xs[i - 1], xRange);
        int y0 = mapY(request.ys[i - 1], yRange);
        int x1 = mapX(request.xs[i], xRange);
        int y1 = mapY(request.ys[i], yRange);
        drawLine(image, x0, y0, x1, y1, kLineColor, kLineColor, 200);
    }
}

void PlotRenderer::drawScatterPlot(Image& image, const PlotRequest& request, const Range& xRange, const Range& yRange) const {
    for (size_t i = 0; i < request.xs.size(); ++i) {
        int x = mapX(request.xs[i], xRange);
        int y = mapY(request.ys[i], yRange);
        drawCircle(image, x, y, 3, kScatterColor, 80, 80);
    }
}

void PlotRenderer::drawBarPlot(Image& image, const PlotRequest& request, const Range& xRange, const Range& yRange) const {
    const size_t n = request.xs.size();
    if (n == 0) {
        return;
    }
    int plotWidth = width_ - kMarginLeft - kMarginRight;
    double barWidth = static_cast<double>(plotWidth) / static_cast<double>(n);

    for (size_t i = 0; i < n; ++i) {
        int xCenter = mapX(request.xs[i], xRange);
        int halfWidth = static_cast<int>(barWidth * 0.4);
        int x0 = xCenter - halfWidth;
        int x1 = xCenter + halfWidth;
        int y0 = mapY(request.ys[i], yRange);
        int y1 = height_ - kMarginBottom;
        drawRect(image, x0, y0, x1, y1, kBarColor, 120, 200);
    }
}

void PlotRenderer::drawLine(Image& image, int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b) const {
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

void PlotRenderer::drawCircle(Image& image, int cx, int cy, int radius, unsigned char r, unsigned char g, unsigned char b) const {
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                image.setPixel(cx + x, cy + y, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawRect(Image& image, int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b) const {
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            image.setPixel(x, y, r, g, b);
        }
    }
}

void PlotRenderer::drawDigit(Image& image, int x, int y, int digit, unsigned char r, unsigned char g, unsigned char b) const {
    static const int font[10][15] = {
        {1,1,1,1,0,1,1,0,1,1,1,1,1,1,1}, // 0
        {0,1,0,1,1,0,0,1,0,0,1,0,1,1,1}, // 1
        {1,1,1,0,0,1,1,1,1,1,0,0,1,1,1}, // 2
        {1,1,1,0,0,1,1,1,1,0,0,1,1,1,1}, // 3
        {1,0,1,1,0,1,1,1,1,0,0,1,0,0,1}, // 4
        {1,1,1,1,0,0,1,1,1,0,0,1,1,1,1}, // 5
        {1,1,1,1,0,0,1,1,1,1,0,1,1,1,1}, // 6
        {1,1,1,0,0,1,0,1,0,0,1,0,0,1,0}, // 7
        {1,1,1,1,0,1,1,1,1,1,0,1,1,1,1}, // 8
        {1,1,1,1,0,1,1,1,1,0,0,1,1,1,1}  // 9
    };

    if (digit < 0 || digit > 9) {
        return;
    }

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (font[digit][row * 3 + col]) {
                image.setPixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawText(Image& image, int x, int y, const std::string& text, unsigned char r, unsigned char g, unsigned char b) const {
    int cursor = x;
    for (char c : text) {
        if (c == '-') {
            for (int i = 0; i < 3; ++i) {
                image.setPixel(cursor + i, y + 2, r, g, b);
            }
            cursor += 4;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            drawDigit(image, cursor, y, c - '0', r, g, b);
            cursor += 4;
        } else {
            cursor += 4;
        }
    }
}
