#include "PlotRenderer.h"

#include <algorithm>
#include <cmath>
#include <string>

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width),
      height_(height),
      marginLeft_(70),
      marginRight_(30),
      marginTop_(40),
      marginBottom_(70) {}

PlotRenderer::Range PlotRenderer::findMinMax(const std::vector<double>& values) const {
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
        minVal -= 1.0;
        maxVal += 1.0;
    }
    return {minVal, maxVal};
}

int PlotRenderer::mapX(double value, const Range& range) const {
    const double span = range.max - range.min;
    const double normalized = (span <= 0.0) ? 0.0 : std::clamp((value - range.min) / span, 0.0, 1.0);
    const int plotWidth = width_ - marginLeft_ - marginRight_;
    return marginLeft_ + static_cast<int>(std::round(normalized * plotWidth));
}

int PlotRenderer::mapY(double value, const Range& range) const {
    const double span = range.max - range.min;
    const double normalized = (span <= 0.0) ? 0.0 : std::clamp((value - range.min) / span, 0.0, 1.0);
    const int plotHeight = height_ - marginTop_ - marginBottom_;
    return height_ - marginBottom_ - static_cast<int>(std::round(normalized * plotHeight));
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

void PlotRenderer::drawCircle(Image& image, int cx, int cy, int radius,
                              unsigned char r, unsigned char g, unsigned char b) const {
    const int radiusSq = radius * radius;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy <= radiusSq) {
                image.setPixel(cx + dx, cy + dy, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image& image, int cx, int cy, int size,
                              unsigned char r, unsigned char g, unsigned char b) const {
    const int half = size / 2;
    for (int y = cy - half; y <= cy + half; ++y) {
        for (int x = cx - half; x <= cx + half; ++x) {
            image.setPixel(x, y, r, g, b);
        }
    }
}

void PlotRenderer::drawGrid(Image& image, const Range& xRange, const Range& yRange) const {
    const unsigned char gridColor = 210;
    const int plotTop = marginTop_;
    const int plotBottom = height_ - marginBottom_;
    const int plotLeft = marginLeft_;
    const int plotRight = width_ - marginRight_;

    if (xRange.max > xRange.min) {
        int start = static_cast<int>(std::ceil(xRange.min));
        int end = static_cast<int>(std::floor(xRange.max));
        if (start == end) {
            end = start + 1;
        }
        for (int value = start; value <= end; ++value) {
            const int x = mapX(static_cast<double>(value), xRange);
            drawLine(image, x, plotTop, x, plotBottom, gridColor, gridColor, gridColor);
        }
    }

    if (yRange.max > yRange.min) {
        int start = static_cast<int>(std::ceil(yRange.min));
        int end = static_cast<int>(std::floor(yRange.max));
        if (start == end) {
            end = start + 1;
        }
        for (int value = start; value <= end; ++value) {
            const int y = mapY(static_cast<double>(value), yRange);
            drawLine(image, plotLeft, y, plotRight, y, gridColor, gridColor, gridColor);
        }
    }
}

void PlotRenderer::drawAxes(Image& image) const {
    const int plotBottom = height_ - marginBottom_;
    const int plotLeft = marginLeft_;
    const int plotRight = width_ - marginRight_;
    const int plotTop = marginTop_;
    drawLine(image, plotLeft, plotTop, plotLeft, plotBottom, 0, 0, 0);
    drawLine(image, plotLeft, plotBottom, plotRight, plotBottom, 0, 0, 0);
}

void PlotRenderer::drawAxisLabels(Image& image, const Range& xRange, const Range& yRange) const {
    const int plotBottom = height_ - marginBottom_;
    const int plotLeft = marginLeft_;

    auto drawValue = [&](int value, int x, int y) {
        const std::string label = std::to_string(value);
        drawText(image, x, y, label, 50, 50, 50);
    };

    if (xRange.max > xRange.min) {
        int start = static_cast<int>(std::ceil(xRange.min));
        int end = static_cast<int>(std::floor(xRange.max));
        if (start == end) {
            end = start + 1;
        }
        for (int value = start; value <= end; ++value) {
            const int x = mapX(static_cast<double>(value), xRange);
            drawLine(image, x, plotBottom, x, plotBottom + 5, 0, 0, 0);
            drawValue(value, x - 5, plotBottom + 15);
        }
    }

    if (yRange.max > yRange.min) {
        int start = static_cast<int>(std::ceil(yRange.min));
        int end = static_cast<int>(std::floor(yRange.max));
        if (start == end) {
            end = start + 1;
        }
        for (int value = start; value <= end; ++value) {
            const int y = mapY(static_cast<double>(value), yRange);
            drawLine(image, plotLeft - 5, y, plotLeft, y, 0, 0, 0);
            drawValue(value, plotLeft - 40, y - 3);
        }
    }
}

bool PlotRenderer::drawCharacter(Image& image, int x, int y, char c,
                                 unsigned char r, unsigned char g, unsigned char b) const {
    static const char* digits[10][5] = {
        {"###", "# #", "# #", "# #", "###"},
        {" ##", "  #", "  #", "  #", " ###"},
        {"###", "  #", "###", "#  ", "###"},
        {"###", "  #", " ##", "  #", "###"},
        {"# #", "# #", "###", "  #", "  #"},
        {"###", "#  ", "###", "  #", "###"},
        {"###", "#  ", "###", "# #", "###"},
        {"###", "  #", "  #", "  #", "  #"},
        {"###", "# #", "###", "# #", "###"},
        {"###", "# #", "###", "  #", "###"}
    };
    static const char* minusPattern[5] = {"   ", "   ", "###", "   ", "   "};
    static const char* dotPattern[5] = {"   ", "   ", "   ", "   ", "##"};

    const char* const* pattern = nullptr;
    if (c >= '0' && c <= '9') {
        pattern = digits[c - '0'];
    } else if (c == '-') {
        pattern = minusPattern;
    } else if (c == '.') {
        pattern = dotPattern;
    } else {
        return false;
    }

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (pattern[row][col] == '#') {
                image.setPixel(x + col, y + row, r, g, b);
            }
        }
    }
    return true;
}

void PlotRenderer::drawText(Image& image, int x, int y, const std::string& text,
                            unsigned char r, unsigned char g, unsigned char b) const {
    int cursor = x;
    for (char c : text) {
        if (drawCharacter(image, cursor, y, c, r, g, b)) {
            cursor += 4;
        } else {
            cursor += 2;
        }
    }
}

Image PlotRenderer::renderLinePlot(const std::vector<double>& xValues, const std::vector<double>& yValues) {
    Range xRange = findMinMax(xValues);
    Range yRange = findMinMax(yValues);
    Image image(width_, height_);
    image.clear(255, 255, 255);
    drawGrid(image, xRange, yRange);
    drawAxes(image);
    drawAxisLabels(image, xRange, yRange);

    for (size_t i = 1; i < xValues.size() && i < yValues.size(); ++i) {
        const int x0 = mapX(xValues[i - 1], xRange);
        const int y0 = mapY(yValues[i - 1], yRange);
        const int x1 = mapX(xValues[i], xRange);
        const int y1 = mapY(yValues[i], yRange);
        drawLine(image, x0, y0, x1, y1, 30, 100, 200);
    }

    return image;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double>& xValues, const std::vector<double>& yValues) {
    Range xRange = findMinMax(xValues);
    Range yRange = findMinMax(yValues);
    Image image(width_, height_);
    image.clear(255, 255, 255);
    drawGrid(image, xRange, yRange);
    drawAxes(image);
    drawAxisLabels(image, xRange, yRange);

    for (size_t i = 0; i < xValues.size() && i < yValues.size(); ++i) {
        const int x = mapX(xValues[i], xRange);
        const int y = mapY(yValues[i], yRange);
        drawCircle(image, x, y, 4, 200, 60, 80);
        drawSquare(image, x, y, 3, 255, 255, 255);
    }

    return image;
}

Image PlotRenderer::renderBarPlot(const std::vector<double>& xValues, const std::vector<double>& yValues) {
    Range xRange = findMinMax(xValues);
    Range yRange = findMinMax(yValues);
    Image image(width_, height_);
    image.clear(255, 255, 255);
    drawGrid(image, xRange, yRange);
    drawAxes(image);
    drawAxisLabels(image, xRange, yRange);

    const int availableWidth = width_ - marginLeft_ - marginRight_;
    const int barWidth = xValues.empty() ? 10 : std::max(6, availableWidth / static_cast<int>(xValues.size() * 2));
    const int zeroY = mapY(0.0, yRange);

    for (size_t i = 0; i < xValues.size() && i < yValues.size(); ++i) {
        const int centerX = mapX(xValues[i], xRange);
        const int valueY = mapY(yValues[i], yRange);
        const int left = centerX - barWidth / 2;
        const int right = centerX + barWidth / 2;
        const int top = std::min(valueY, zeroY);
        const int bottom = std::max(valueY, zeroY);
        for (int x = left; x <= right; ++x) {
            for (int y = top; y <= bottom; ++y) {
                image.setPixel(x, y, 80, 180, 90);
            }
        }
    }

    return image;
}
