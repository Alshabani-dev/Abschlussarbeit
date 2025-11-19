#include "PlotRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace {

const PlotRenderer::Color kBackground{245, 249, 255};
const PlotRenderer::Color kAxisColor{30, 30, 30};
const PlotRenderer::Color kGridColor{215, 225, 240};
const PlotRenderer::Color kLabelColor{30, 30, 30};
const PlotRenderer::Color kLineColor{53, 101, 189};
const PlotRenderer::Color kScatterColor{196, 81, 86};
const PlotRenderer::Color kBarColor{66, 168, 107};
const PlotRenderer::Color kBarOutline{49, 120, 79};

const std::unordered_map<char, std::array<std::string, 5>> kGlyphs{
    {'0', {"###", "# #", "# #", "# #", "###"}},
    {'1', {" ##", "# #", "  #", "  #", "#####"}},
    {'2', {"###", "  #", "###", "#  ", "###"}},
    {'3', {"###", "  #", " ##", "  #", "###"}},
    {'4', {"# #", "# #", "###", "  #", "  #"}},
    {'5', {"###", "#  ", "###", "  #", "###"}},
    {'6', {"###", "#  ", "###", "# #", "###"}},
    {'7', {"###", "  #", "  #", " # ", " # "}},
    {'8', {"###", "# #", "###", "# #", "###"}},
    {'9', {"###", "# #", "###", "  #", "###"}},
    {'-', {"   ", "   ", "###", "   ", "   "}},
    {'.', {"   ", "   ", "   ", "   ", "###"}}
};

} // namespace

PlotRenderer::PlotRenderer(int width, int height) : width_(width), height_(height) {}

Image PlotRenderer::renderLinePlot(const std::vector<double> &x, const std::vector<double> &y) {
    Image image = basePlot(x, y);
    if (x.empty() || y.empty()) {
        return image;
    }

    for (size_t i = 1; i < x.size() && i < y.size(); ++i) {
        int x0 = mapX(x[i - 1]);
        int y0 = mapY(y[i - 1]);
        int x1 = mapX(x[i]);
        int y1 = mapY(y[i]);
        drawLine(image, x0, y0, x1, y1, kLineColor);
    }

    return image;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double> &x, const std::vector<double> &y) {
    Image image = basePlot(x, y);
    for (size_t i = 0; i < x.size() && i < y.size(); ++i) {
        drawCircle(image, mapX(x[i]), mapY(y[i]), 4, kScatterColor);
    }
    return image;
}

Image PlotRenderer::renderBarPlot(const std::vector<double> &x, const std::vector<double> &y) {
    Image image = basePlot(x, y);
    if (x.empty() || y.empty()) {
        return image;
    }

    int plotWidth = width_ - marginLeft_ - marginRight_;
    int barWidth = std::max(6, plotWidth / std::max<int>(static_cast<int>(x.size()) * 2, 1));
    double baseValue;
    if (minY_ > 0) {
        baseValue = minY_;
    } else if (maxY_ < 0) {
        baseValue = maxY_;
    } else {
        baseValue = 0.0;
    }
    int baseY = mapY(baseValue);

    for (size_t i = 0; i < x.size() && i < y.size(); ++i) {
        int centerX = mapX(x[i]);
        int left = centerX - barWidth / 2;
        int right = centerX + barWidth / 2;
        int top = mapY(y[i]);
        int yStart = std::min(baseY, top);
        int yEnd = std::max(baseY, top);

        for (int px = left; px <= right; ++px) {
            for (int py = yStart; py <= yEnd; ++py) {
                image.setPixel(px, py, kBarColor.r, kBarColor.g, kBarColor.b);
            }
            image.setPixel(px, yStart, kBarOutline.r, kBarOutline.g, kBarOutline.b);
            image.setPixel(px, yEnd, kBarOutline.r, kBarOutline.g, kBarOutline.b);
        }
    }

    return image;
}

void PlotRenderer::computeBounds(const std::vector<double> &x, const std::vector<double> &y) {
    auto compute = [](const std::vector<double> &values) -> std::pair<double, double> {
        if (values.empty()) {
            return {0.0, 1.0};
        }
        double minVal = std::numeric_limits<double>::infinity();
        double maxVal = -std::numeric_limits<double>::infinity();
        for (double v : values) {
            minVal = std::min(minVal, v);
            maxVal = std::max(maxVal, v);
        }
        if (minVal == maxVal) {
            minVal -= 1.0;
            maxVal += 1.0;
        }
        return {minVal, maxVal};
    };

    std::vector<double> xValues = x;
    if (xValues.empty()) {
        xValues.resize(y.size());
        for (size_t i = 0; i < y.size(); ++i) {
            xValues[i] = static_cast<double>(i);
        }
    }

    auto [minX, maxX] = compute(xValues);
    auto [minY, maxY] = compute(y);

    minX_ = minX;
    maxX_ = maxX;
    minY_ = minY;
    maxY_ = maxY;
}

int PlotRenderer::mapX(double value) const {
    int plotWidth = width_ - marginLeft_ - marginRight_;
    if (plotWidth <= 0) {
        return marginLeft_;
    }
    double normalized = (value - minX_) / (maxX_ - minX_);
    normalized = std::clamp(normalized, 0.0, 1.0);
    return marginLeft_ + static_cast<int>(normalized * plotWidth + 0.5);
}

int PlotRenderer::mapY(double value) const {
    int plotHeight = height_ - marginTop_ - marginBottom_;
    if (plotHeight <= 0) {
        return height_ - marginBottom_;
    }
    double normalized = (value - minY_) / (maxY_ - minY_);
    normalized = std::clamp(normalized, 0.0, 1.0);
    int plotBottom = height_ - marginBottom_;
    return plotBottom - static_cast<int>(normalized * plotHeight + 0.5);
}

void PlotRenderer::drawLine(Image &image, int x0, int y0, int x1, int y1, Color color) const {
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
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

void PlotRenderer::drawCircle(Image &image, int cx, int cy, int radius, Color color) const {
    int radiusSq = radius * radius;
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radiusSq) {
                image.setPixel(cx + x, cy + y, color.r, color.g, color.b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image &image, int cx, int cy, int size, Color color) const {
    int half = size / 2;
    for (int y = -half; y <= half; ++y) {
        for (int x = -half; x <= half; ++x) {
            image.setPixel(cx + x, cy + y, color.r, color.g, color.b);
        }
    }
}

void PlotRenderer::drawDigit(Image &image, int x, int y, char digit, Color color) const {
    auto it = kGlyphs.find(digit);
    if (it == kGlyphs.end()) {
        return;
    }
    const auto &glyph = it->second;
    for (size_t row = 0; row < glyph.size(); ++row) {
        for (size_t col = 0; col < glyph[row].size(); ++col) {
            if (glyph[row][col] == '#') {
                image.setPixel(x + static_cast<int>(col), y + static_cast<int>(row), color.r, color.g, color.b);
            }
        }
    }
}

void PlotRenderer::drawText(Image &image, int x, int y, const std::string &text, Color color) const {
    int cursor = x;
    for (char ch : text) {
        if (ch == ' ') {
            cursor += 4;
            continue;
        }
        drawDigit(image, cursor, y, ch, color);
        cursor += 4;
    }
}

std::string PlotRenderer::formatNumber(double value) const {
    std::ostringstream stream;
    stream.setf(std::ios::fixed);
    stream << std::setprecision(2) << value;
    std::string result = stream.str();
    while (!result.empty() && result.back() == '0') {
        result.pop_back();
    }
    if (!result.empty() && result.back() == '.') {
        result.pop_back();
    }
    if (result.empty()) {
        result = "0";
    }
    return result;
}

void PlotRenderer::drawBaseline(Image &image) const {
    int left = marginLeft_;
    int right = width_ - marginRight_;
    int top = marginTop_;
    int bottom = height_ - marginBottom_;
    for (int x = left; x <= right; ++x) {
        image.setPixel(x, top, kAxisColor.r, kAxisColor.g, kAxisColor.b);
        image.setPixel(x, bottom, kAxisColor.r, kAxisColor.g, kAxisColor.b);
    }
    for (int y = top; y <= bottom; ++y) {
        image.setPixel(left, y, kAxisColor.r, kAxisColor.g, kAxisColor.b);
        image.setPixel(right, y, kAxisColor.r, kAxisColor.g, kAxisColor.b);
    }
}

void PlotRenderer::drawAxes(Image &image) const {
    drawBaseline(image);

    if (minY_ < 0 && maxY_ > 0) {
        int zeroY = mapY(0.0);
        for (int x = marginLeft_; x <= width_ - marginRight_; ++x) {
            image.setPixel(x, zeroY, kAxisColor.r, kAxisColor.g, kAxisColor.b);
        }
    }

    if (minX_ < 0 && maxX_ > 0) {
        int zeroX = mapX(0.0);
        for (int y = marginTop_; y <= height_ - marginBottom_; ++y) {
            image.setPixel(zeroX, y, kAxisColor.r, kAxisColor.g, kAxisColor.b);
        }
    }
}

void PlotRenderer::drawGrid(Image &image) const {
    int plotLeft = marginLeft_;
    int plotRight = width_ - marginRight_;
    int plotTop = marginTop_;
    int plotBottom = height_ - marginBottom_;

    int startX = static_cast<int>(std::ceil(minX_));
    int endX = static_cast<int>(std::floor(maxX_));
    if (startX > endX) {
        startX = static_cast<int>(std::floor(minX_));
        endX = static_cast<int>(std::ceil(maxX_));
    }

    for (int value = startX; value <= endX; ++value) {
        int x = mapX(static_cast<double>(value));
        for (int y = plotTop; y <= plotBottom; ++y) {
            image.setPixel(x, y, kGridColor.r, kGridColor.g, kGridColor.b);
        }
    }

    int startY = static_cast<int>(std::ceil(minY_));
    int endY = static_cast<int>(std::floor(maxY_));
    if (startY > endY) {
        startY = static_cast<int>(std::floor(minY_));
        endY = static_cast<int>(std::ceil(maxY_));
    }

    for (int value = startY; value <= endY; ++value) {
        int y = mapY(static_cast<double>(value));
        for (int x = plotLeft; x <= plotRight; ++x) {
            image.setPixel(x, y, kGridColor.r, kGridColor.g, kGridColor.b);
        }
    }
}

void PlotRenderer::drawAxisLabels(Image &image) const {
    int plotBottom = height_ - marginBottom_ + 10;

    int startX = static_cast<int>(std::ceil(minX_));
    int endX = static_cast<int>(std::floor(maxX_));
    if (startX > endX) {
        startX = static_cast<int>(std::floor(minX_));
        endX = static_cast<int>(std::ceil(maxX_));
    }
    for (int value = startX; value <= endX; ++value) {
        std::string label = formatNumber(static_cast<double>(value));
        int textWidth = static_cast<int>(label.size()) * 4;
        int x = mapX(static_cast<double>(value)) - textWidth / 2;
        drawText(image, x, plotBottom, label, kLabelColor);
    }

    int startY = static_cast<int>(std::ceil(minY_));
    int endY = static_cast<int>(std::floor(maxY_));
    if (startY > endY) {
        startY = static_cast<int>(std::floor(minY_));
        endY = static_cast<int>(std::ceil(maxY_));
    }
    for (int value = startY; value <= endY; ++value) {
        std::string label = formatNumber(static_cast<double>(value));
        int textWidth = static_cast<int>(label.size()) * 4;
        int y = mapY(static_cast<double>(value)) - 3;
        drawText(image, marginLeft_ - textWidth - 10, y, label, kLabelColor);
    }
}

Image PlotRenderer::basePlot(const std::vector<double> &x, const std::vector<double> &y) {
    computeBounds(x, y);
    Image image(width_, height_);
    image.clear(kBackground.r, kBackground.g, kBackground.b);
    drawGrid(image);
    drawAxes(image);
    drawAxisLabels(image);
    return image;
}
