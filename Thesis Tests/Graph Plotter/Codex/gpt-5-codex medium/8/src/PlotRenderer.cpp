#include "PlotRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <sstream>

namespace {

using Pattern = std::array<std::string, 5>;

const Pattern &digitPattern(char digit) {
    static const Pattern zero = {"111", "101", "101", "101", "111"};
    static const Pattern one = {"010", "110", "010", "010", "111"};
    static const Pattern two = {"111", "001", "111", "100", "111"};
    static const Pattern three = {"111", "001", "111", "001", "111"};
    static const Pattern four = {"101", "101", "111", "001", "001"};
    static const Pattern five = {"111", "100", "111", "001", "111"};
    static const Pattern six = {"111", "100", "111", "101", "111"};
    static const Pattern seven = {"111", "001", "010", "010", "010"};
    static const Pattern eight = {"111", "101", "111", "101", "111"};
    static const Pattern nine = {"111", "101", "111", "001", "111"};
    static const Pattern minus = {"000", "000", "111", "000", "000"};
    static const Pattern dot = {"000", "000", "000", "000", "010"};
    static const Pattern blank = {"000", "000", "000", "000", "000"};

    switch (digit) {
    case '0':
        return zero;
    case '1':
        return one;
    case '2':
        return two;
    case '3':
        return three;
    case '4':
        return four;
    case '5':
        return five;
    case '6':
        return six;
    case '7':
        return seven;
    case '8':
        return eight;
    case '9':
        return nine;
    case '-':
        return minus;
    case '.':
        return dot;
    default:
        break;
    }
    return blank;
}

} // namespace

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width),
      height_(height),
      marginLeft_(70),
      marginRight_(30),
      marginTop_(30),
      marginBottom_(60) {}

PlotRenderer::Range PlotRenderer::findMinMax(const std::vector<double> &values) const {
    Range range;
    if (values.empty()) {
        range.min = 0.0;
        range.max = 1.0;
        return range;
    }
    range.min = std::numeric_limits<double>::max();
    range.max = std::numeric_limits<double>::lowest();
    for (double value : values) {
        range.min = std::min(range.min, value);
        range.max = std::max(range.max, value);
    }
    if (std::abs(range.max - range.min) < 1e-9) {
        range.min -= 1.0;
        range.max += 1.0;
    }
    return range;
}

int PlotRenderer::mapX(double value, const Range &range) const {
    double denominator = range.max - range.min;
    double t = denominator < 1e-9 ? 0.5 : (value - range.min) / denominator;
    t = std::clamp(t, 0.0, 1.0);
    int plotWidth = std::max(1, width_ - marginLeft_ - marginRight_);
    double px = marginLeft_ + t * (plotWidth - 1);
    return static_cast<int>(std::round(px));
}

int PlotRenderer::mapY(double value, const Range &range) const {
    double denominator = range.max - range.min;
    double t = denominator < 1e-9 ? 0.5 : (value - range.min) / denominator;
    t = std::clamp(t, 0.0, 1.0);
    int plotHeight = std::max(1, height_ - marginTop_ - marginBottom_);
    double py = marginTop_ + (1.0 - t) * (plotHeight - 1);
    return static_cast<int>(std::round(py));
}

void PlotRenderer::drawLine(Image &image, int x0, int y0, int x1, int y1, Color color) {
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        image.setPixel(x0, y0, color.r, color.g, color.b);
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

void PlotRenderer::drawCircle(Image &image, int cx, int cy, int radius, Color color) {
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                image.setPixel(cx + x, cy + y, color.r, color.g, color.b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image &image, int cx, int cy, int size, Color color) {
    int half = size / 2;
    for (int y = cy - half; y <= cy + half; ++y) {
        for (int x = cx - half; x <= cx + half; ++x) {
            image.setPixel(x, y, color.r, color.g, color.b);
        }
    }
}

void PlotRenderer::fillRect(Image &image, int x0, int y0, int x1, int y1, Color color) {
    if (x0 > x1) {
        std::swap(x0, x1);
    }
    if (y0 > y1) {
        std::swap(y0, y1);
    }
    x0 = std::clamp(x0, 0, width_ - 1);
    x1 = std::clamp(x1, 0, width_ - 1);
    y0 = std::clamp(y0, 0, height_ - 1);
    y1 = std::clamp(y1, 0, height_ - 1);

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            image.setPixel(x, y, color.r, color.g, color.b);
        }
    }
}

void PlotRenderer::drawDigit(Image &image, int x, int y, char digit, Color color) {
    const Pattern &pattern = digitPattern(digit);
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (pattern[row][col] == '1') {
                image.setPixel(x + col, y + row, color.r, color.g, color.b);
            }
        }
    }
}

int PlotRenderer::measureTextWidth(const std::string &text) const {
    if (text.empty()) {
        return 0;
    }
    return static_cast<int>(text.size()) * 4 - 1;
}

void PlotRenderer::drawText(Image &image, int x, int y, const std::string &text, Color color) {
    int offsetX = x;
    for (char c : text) {
        drawDigit(image, offsetX, y, c, color);
        offsetX += 4;
    }
}

void PlotRenderer::drawGrid(Image &image, const Range &xRange, const Range &yRange) {
    Color gridColor{220, 230, 240};
    int top = marginTop_;
    int bottom = height_ - marginBottom_;
    int left = marginLeft_;
    int right = width_ - marginRight_;

    int startX = static_cast<int>(std::ceil(xRange.min));
    int endX = static_cast<int>(std::floor(xRange.max));
    if (startX > endX) {
        startX = static_cast<int>(std::floor(xRange.min));
        endX = startX;
    }
    for (int value = startX; value <= endX; ++value) {
        int x = mapX(static_cast<double>(value), xRange);
        for (int y = top; y <= bottom; ++y) {
            image.setPixel(x, y, gridColor.r, gridColor.g, gridColor.b);
        }
    }

    int startY = static_cast<int>(std::ceil(yRange.min));
    int endY = static_cast<int>(std::floor(yRange.max));
    if (startY > endY) {
        startY = static_cast<int>(std::floor(yRange.min));
        endY = startY;
    }
    for (int value = startY; value <= endY; ++value) {
        int y = mapY(static_cast<double>(value), yRange);
        for (int x = left; x <= right; ++x) {
            image.setPixel(x, y, gridColor.r, gridColor.g, gridColor.b);
        }
    }
}

void PlotRenderer::drawAxes(Image &image, const Range &xRange, const Range &yRange) {
    Color axisColor{60, 60, 60};
    int left = marginLeft_;
    int right = width_ - marginRight_;
    int top = marginTop_;
    int bottom = height_ - marginBottom_;

    drawLine(image, left, top, left, bottom, axisColor);
    drawLine(image, left, bottom, right, bottom, axisColor);

    int zeroX = mapX(0.0, xRange);
    drawLine(image, zeroX, top, zeroX, bottom, axisColor);

    int zeroY = mapY(0.0, yRange);
    drawLine(image, left, zeroY, right, zeroY, axisColor);
}

void PlotRenderer::drawAxisLabels(Image &image, const Range &xRange, const Range &yRange) {
    Color textColor{20, 20, 20};
    int bottom = height_ - marginBottom_ + 8;
    int leftPadding = marginLeft_ - 10;

    int startX = static_cast<int>(std::ceil(xRange.min));
    int endX = static_cast<int>(std::floor(xRange.max));
    if (startX > endX) {
        startX = static_cast<int>(std::floor(xRange.min));
        endX = startX;
    }
    for (int value = startX; value <= endX; ++value) {
        std::string label = std::to_string(value);
        int labelWidth = measureTextWidth(label);
        int x = mapX(static_cast<double>(value), xRange) - labelWidth / 2;
        drawText(image, x, bottom, label, textColor);
    }

    int startY = static_cast<int>(std::ceil(yRange.min));
    int endY = static_cast<int>(std::floor(yRange.max));
    if (startY > endY) {
        startY = static_cast<int>(std::floor(yRange.min));
        endY = startY;
    }
    for (int value = startY; value <= endY; ++value) {
        std::string label = std::to_string(value);
        int labelWidth = measureTextWidth(label);
        int y = mapY(static_cast<double>(value), yRange) - 2;
        drawText(image, leftPadding - labelWidth, y - 3, label, textColor);
    }
}

void PlotRenderer::renderLineSeries(Image &image, const std::vector<int> &xs, const std::vector<int> &ys, Color color) {
    if (xs.size() < 2 || ys.size() < 2) {
        return;
    }
    for (size_t i = 1; i < xs.size(); ++i) {
        drawLine(image, xs[i - 1], ys[i - 1], xs[i], ys[i], color);
    }
}

void PlotRenderer::renderScatterSeries(Image &image, const std::vector<int> &xs, const std::vector<int> &ys, Color color) {
    for (size_t i = 0; i < xs.size(); ++i) {
        drawCircle(image, xs[i], ys[i], 3, color);
    }
}

void PlotRenderer::renderBarSeries(Image &image, const std::vector<int> &xs, const std::vector<int> &ys, double zeroY, Color color) {
    if (xs.empty()) {
        return;
    }
    int plotWidth = std::max(1, width_ - marginLeft_ - marginRight_);
    int barWidth = std::max(4, plotWidth / static_cast<int>(xs.size() * 2));
    for (size_t i = 0; i < xs.size(); ++i) {
        int xCenter = xs[i];
        int x0 = xCenter - barWidth / 2;
        int x1 = xCenter + barWidth / 2;
        int y0 = static_cast<int>(zeroY);
        int y1 = ys[i];
        fillRect(image, x0, y0, x1, y1, color);
    }
}

Image PlotRenderer::renderLinePlot(const std::vector<double> &x, const std::vector<double> &y) {
    return renderPlot(x, y, PlotType::Line);
}

Image PlotRenderer::renderScatterPlot(const std::vector<double> &x, const std::vector<double> &y) {
    return renderPlot(x, y, PlotType::Scatter);
}

Image PlotRenderer::renderBarPlot(const std::vector<double> &x, const std::vector<double> &y) {
    return renderPlot(x, y, PlotType::Bar);
}

Image PlotRenderer::renderPlot(const std::vector<double> &x, const std::vector<double> &y, PlotType type) {
    Image image(width_, height_);
    image.clear(255, 255, 255);

    Range xRange = findMinMax(x);
    Range yRange = findMinMax(y);

    drawGrid(image, xRange, yRange);
    drawAxes(image, xRange, yRange);
    drawAxisLabels(image, xRange, yRange);

    std::vector<int> xs;
    std::vector<int> ys;
    xs.reserve(x.size());
    ys.reserve(y.size());

    for (size_t i = 0; i < x.size() && i < y.size(); ++i) {
        xs.push_back(mapX(x[i], xRange));
        ys.push_back(mapY(y[i], yRange));
    }

    Color lineColor{40, 90, 180};
    Color scatterColor{230, 80, 60};
    Color barColor{100, 160, 90};

    switch (type) {
    case PlotType::Line:
        renderLineSeries(image, xs, ys, lineColor);
        break;
    case PlotType::Scatter:
        renderScatterSeries(image, xs, ys, scatterColor);
        break;
    case PlotType::Bar:
        renderBarSeries(image, xs, ys, mapY(0.0, yRange), barColor);
        break;
    }
    return image;
}
