#include "PlotRenderer.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace {
const unsigned char kAxisR = 40;
const unsigned char kAxisG = 40;
const unsigned char kAxisB = 40;
const unsigned char kGridR = 220;
const unsigned char kGridG = 220;
const unsigned char kGridB = 220;
const unsigned char kTextR = 30;
const unsigned char kTextG = 30;
const unsigned char kTextB = 30;
const unsigned char kLineR = 33;
const unsigned char kLineG = 115;
const unsigned char kLineB = 199;
const unsigned char kScatterR = 200;
const unsigned char kScatterG = 80;
const unsigned char kScatterB = 60;
const unsigned char kBarR = 80;
const unsigned char kBarG = 160;
const unsigned char kBarB = 90;

const unsigned char DIGIT_FONT[12][15] = {
    // Digits 0-9, dash, dot (3x5)
    {1,1,1,1,0,1,1,0,1,1,0,1,1,1,1}, // 0
    {0,1,0,1,1,0,0,1,0,0,1,0,1,1,1}, // 1
    {1,1,1,0,0,1,1,1,1,1,0,0,1,1,1}, // 2
    {1,1,1,0,0,1,0,1,1,0,0,1,1,1,1}, // 3
    {1,0,1,1,0,1,1,1,1,0,0,1,0,0,1}, // 4
    {1,1,1,1,0,0,1,1,1,0,0,1,1,1,1}, // 5
    {1,1,1,1,0,0,1,1,1,1,0,1,1,1,1}, // 6
    {1,1,1,0,0,1,0,0,1,0,0,1,0,0,1}, // 7
    {1,1,1,1,0,1,1,1,1,1,0,1,1,1,1}, // 8
    {1,1,1,1,0,1,1,1,1,0,0,1,1,1,1}, // 9
    {0,0,0,0,0,0,1,1,1,0,0,0,0,0,0}, // -
    {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1}  // .
};
} // namespace

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width), height_(height),
      marginLeft_(70), marginRight_(30), marginTop_(30), marginBottom_(60) {}

PlotRenderer::PlotType PlotRenderer::plotTypeFromString(const std::string& name) {
    if (name == "scatter") {
        return PlotType::Scatter;
    }
    if (name == "bar") {
        return PlotType::Bar;
    }
    return PlotType::Line;
}

PlotRenderer::Bounds PlotRenderer::calculateBounds(const std::vector<double>& xs,
                                                   const std::vector<double>& ys) const {
    Bounds bounds{};
    bounds.minX = *std::min_element(xs.begin(), xs.end());
    bounds.maxX = *std::max_element(xs.begin(), xs.end());
    bounds.minY = *std::min_element(ys.begin(), ys.end());
    bounds.maxY = *std::max_element(ys.begin(), ys.end());

    if (bounds.minX == bounds.maxX) {
        bounds.minX -= 1.0;
        bounds.maxX += 1.0;
    }
    if (bounds.minY == bounds.maxY) {
        bounds.minY -= 1.0;
        bounds.maxY += 1.0;
    }
    return bounds;
}

double PlotRenderer::mapX(double value, const Bounds& bounds) const {
    const double span = bounds.maxX - bounds.minX;
    const double fraction = (value - bounds.minX) / span;
    const double plotWidth = static_cast<double>(width_ - marginLeft_ - marginRight_);
    return marginLeft_ + fraction * plotWidth;
}

double PlotRenderer::mapY(double value, const Bounds& bounds) const {
    const double span = bounds.maxY - bounds.minY;
    const double fraction = (value - bounds.minY) / span;
    const double plotHeight = static_cast<double>(height_ - marginTop_ - marginBottom_);
    return (height_ - marginBottom_) - fraction * plotHeight;
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
    int dx = x1 - x0;
    int dy = std::abs(y1 - y0);
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

void PlotRenderer::drawRectangle(Image& image, int x0, int y0, int x1, int y1,
                                 unsigned char r, unsigned char g, unsigned char b) const {
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            image.setPixel(x, y, r, g, b);
        }
    }
}

void PlotRenderer::drawDigit(Image& image, int x, int y, char digit,
                             unsigned char r, unsigned char g, unsigned char b) const {
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
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (DIGIT_FONT[index][row * 3 + col]) {
                image.setPixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawText(Image& image, int x, int y, const std::string& text,
                            unsigned char r, unsigned char g, unsigned char b) const {
    int cursor = 0;
    for (char c : text) {
        drawDigit(image, x + cursor, y, c, r, g, b);
        cursor += 4;
    }
}

void PlotRenderer::drawAxes(Image& image, const Bounds& bounds) const {
    (void)bounds;
    const int left = marginLeft_;
    const int right = width_ - marginRight_;
    const int top = marginTop_;
    const int bottom = height_ - marginBottom_;
    drawLine(image, left, bottom, right, bottom, kAxisR, kAxisG, kAxisB);
    drawLine(image, left, top, left, bottom, kAxisR, kAxisG, kAxisB);
}

void PlotRenderer::drawGridAndLabels(Image& image, const Bounds& bounds) const {
    const int left = marginLeft_;
    const int right = width_ - marginRight_;
    const int top = marginTop_;
    const int bottom = height_ - marginBottom_;

    const int minX = static_cast<int>(std::ceil(bounds.minX));
    const int maxX = static_cast<int>(std::floor(bounds.maxX));
    for (int value = minX; value <= maxX; ++value) {
        const int xPixel = static_cast<int>(std::round(mapX(value, bounds)));
        drawLine(image, xPixel, top, xPixel, bottom, kGridR, kGridG, kGridB);
        drawText(image, xPixel - 6, bottom + 10, std::to_string(value), kTextR, kTextG, kTextB);
    }

    const int minY = static_cast<int>(std::ceil(bounds.minY));
    const int maxY = static_cast<int>(std::floor(bounds.maxY));
    for (int value = minY; value <= maxY; ++value) {
        const int yPixel = static_cast<int>(std::round(mapY(value, bounds)));
        drawLine(image, left, yPixel, right, yPixel, kGridR, kGridG, kGridB);
        drawText(image, left - 30, yPixel - 2, std::to_string(value), kTextR, kTextG, kTextB);
    }
}

Image PlotRenderer::renderLinePlot(const std::vector<double>& xs,
                                   const std::vector<double>& ys,
                                   const Bounds& bounds) {
    Image image(width_, height_);
    image.clear(255, 255, 255);
    drawGridAndLabels(image, bounds);
    drawAxes(image, bounds);

    for (size_t i = 1; i < xs.size(); ++i) {
        int x0 = static_cast<int>(std::round(mapX(xs[i - 1], bounds)));
        int y0 = static_cast<int>(std::round(mapY(ys[i - 1], bounds)));
        int x1 = static_cast<int>(std::round(mapX(xs[i], bounds)));
        int y1 = static_cast<int>(std::round(mapY(ys[i], bounds)));
        drawLine(image, x0, y0, x1, y1, kLineR, kLineG, kLineB);
    }

    return image;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double>& xs,
                                      const std::vector<double>& ys,
                                      const Bounds& bounds) {
    Image image(width_, height_);
    image.clear(255, 255, 255);
    drawGridAndLabels(image, bounds);
    drawAxes(image, bounds);

    for (size_t i = 0; i < xs.size(); ++i) {
        int x = static_cast<int>(std::round(mapX(xs[i], bounds)));
        int y = static_cast<int>(std::round(mapY(ys[i], bounds)));
        drawCircle(image, x, y, 4, kScatterR, kScatterG, kScatterB);
    }
    return image;
}

Image PlotRenderer::renderBarPlot(const std::vector<double>& xs,
                                  const std::vector<double>& ys,
                                  const Bounds& bounds) {
    Image image(width_, height_);
    image.clear(255, 255, 255);
    drawGridAndLabels(image, bounds);
    drawAxes(image, bounds);

    const double spanX = bounds.maxX - bounds.minX;
    const double plotWidth = static_cast<double>(width_ - marginLeft_ - marginRight_);
    double pixelWidth = plotWidth / std::max<size_t>(xs.size(), 1);
    if (xs.size() > 1) {
        double minDistance = spanX;
        for (size_t i = 1; i < xs.size(); ++i) {
            minDistance = std::min(minDistance, std::abs(xs[i] - xs[i - 1]));
        }
        pixelWidth = (minDistance / spanX) * plotWidth * 0.8;
    }
    int halfWidth = std::max(3, static_cast<int>(pixelWidth / 2));

    const int baseline = static_cast<int>(std::round(mapY(0.0, bounds)));
    for (size_t i = 0; i < xs.size(); ++i) {
        int xCenter = static_cast<int>(std::round(mapX(xs[i], bounds)));
        int yValue = static_cast<int>(std::round(mapY(ys[i], bounds)));
        drawRectangle(image, xCenter - halfWidth, baseline, xCenter + halfWidth, yValue,
                      kBarR, kBarG, kBarB);
    }
    return image;
}

Image PlotRenderer::renderPlot(const std::vector<double>& xs,
                               const std::vector<double>& ys,
                               PlotType type) {
    Bounds bounds = calculateBounds(xs, ys);
    switch (type) {
        case PlotType::Scatter:
            return renderScatterPlot(xs, ys, bounds);
        case PlotType::Bar:
            return renderBarPlot(xs, ys, bounds);
        case PlotType::Line:
        default:
            return renderLinePlot(xs, ys, bounds);
    }
}
