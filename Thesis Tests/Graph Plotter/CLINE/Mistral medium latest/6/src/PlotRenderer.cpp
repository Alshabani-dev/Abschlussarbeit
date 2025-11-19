#include "PlotRenderer.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width), height_(height),
      marginLeft_(80), marginRight_(40),
      marginTop_(40), marginBottom_(80) {}

void PlotRenderer::drawLine(Image& img, int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b) {
    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        img.setPixel(x1, y1, r, g, b);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void PlotRenderer::drawCircle(Image& img, int cx, int cy, int radius, unsigned char r, unsigned char g, unsigned char b) {
    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            if (x * x + y * y <= radius * radius) {
                img.setPixel(cx + x, cy + y, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image& img, int cx, int cy, int size, unsigned char r, unsigned char g, unsigned char b) {
    int halfSize = size / 2;
    for (int x = cx - halfSize; x <= cx + halfSize; ++x) {
        for (int y = cy - halfSize; y <= cy + halfSize; ++y) {
            img.setPixel(x, y, r, g, b);
        }
    }
}

void PlotRenderer::drawDigit(Image& img, int x, int y, char digit, unsigned char r, unsigned char g, unsigned char b) {
    // Simple 3x5 bitmap font for digits 0-9
    const unsigned char font[10][5] = {
        {0x7E, 0x81, 0x81, 0x81, 0x7E}, // 0
        {0x00, 0x41, 0x7F, 0x41, 0x00}, // 1
        {0x72, 0x89, 0x89, 0x89, 0x76}, // 2
        {0x42, 0x81, 0x81, 0x81, 0x7E}, // 3
        {0xF8, 0x10, 0x10, 0x10, 0x1F}, // 4
        {0x4F, 0x89, 0x89, 0x89, 0x71}, // 5
        {0x7E, 0x89, 0x89, 0x89, 0x70}, // 6
        {0x40, 0x47, 0x48, 0x50, 0x60}, // 7
        {0x7E, 0x89, 0x89, 0x89, 0x7E}, // 8
        {0x4F, 0x89, 0x89, 0x89, 0x7F}  // 9
    };

    if (digit >= '0' && digit <= '9') {
        int digitIndex = digit - '0';
        for (int row = 0; row < 5; ++row) {
            unsigned char bits = font[digitIndex][row];
            for (int col = 0; col < 3; ++col) {
                if (bits & (1 << (2 - col))) {
                    img.setPixel(x + col, y + row, r, g, b);
                }
            }
        }
    } else if (digit == '-') {
        // Draw minus sign
        for (int col = 0; col < 3; ++col) {
            img.setPixel(x + col, y + 2, r, g, b);
        }
    }
}

void PlotRenderer::drawText(Image& img, int x, int y, const std::string& text, unsigned char r, unsigned char g, unsigned char b) {
    for (size_t i = 0; i < text.size(); ++i) {
        drawDigit(img, x + i * 4, y, text[i], r, g, b);
    }
}

void PlotRenderer::drawAxes(Image& img) {
    // Draw X axis
    drawLine(img, marginLeft_, height_ - marginBottom_,
             width_ - marginRight_, height_ - marginBottom_,
             0, 0, 0);

    // Draw Y axis
    drawLine(img, marginLeft_, marginTop_,
             marginLeft_, height_ - marginBottom_,
             0, 0, 0);
}

void PlotRenderer::drawGrid(Image& img, double xMin, double xMax, double yMin, double yMax) {
    // Draw grid lines at each integer position
    int xMinInt = static_cast<int>(std::ceil(xMin));
    int xMaxInt = static_cast<int>(std::floor(xMax));
    int yMinInt = static_cast<int>(std::ceil(yMin));
    int yMaxInt = static_cast<int>(std::floor(yMax));

    // Vertical grid lines at each X integer
    for (int xi = xMinInt; xi <= xMaxInt; ++xi) {
        int x = mapX(xi, xMin, xMax);
        drawLine(img, x, marginTop_, x, height_ - marginBottom_, 220, 220, 220);
    }

    // Horizontal grid lines at each Y integer
    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        drawLine(img, marginLeft_, y, width_ - marginRight_, y, 220, 220, 220);
    }
}

void PlotRenderer::drawAxisLabels(Image& img, double xMin, double xMax, double yMin, double yMax) {
    // Draw label at each integer position
    int xMinInt = static_cast<int>(std::ceil(xMin));
    int xMaxInt = static_cast<int>(std::floor(xMax));
    int yMinInt = static_cast<int>(std::ceil(yMin));
    int yMaxInt = static_cast<int>(std::floor(yMax));

    // X-axis labels
    for (int xi = xMinInt; xi <= xMaxInt; ++xi) {
        int x = mapX(xi, xMin, xMax);
        std::string label = std::to_string(xi);
        drawText(img, x - label.length() * 2, height_ - marginBottom_ + 10, label, 0, 0, 0);
    }

    // Y-axis labels
    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        std::string label = std::to_string(yi);
        drawText(img, marginLeft_ - 30, y - 2, label, 0, 0, 0);
    }
}

int PlotRenderer::mapX(double xValue, double xMin, double xMax) const {
    int plotWidth = width_ - marginLeft_ - marginRight_;
    double normalized = (xValue - xMin) / (xMax - xMin);
    return marginLeft_ + static_cast<int>(normalized * plotWidth);
}

int PlotRenderer::mapY(double yValue, double yMin, double yMax) const {
    int plotHeight = height_ - marginTop_ - marginBottom_;
    double normalized = (yValue - yMin) / (yMax - yMin);
    // Invert Y axis (higher values at top)
    return height_ - marginBottom_ - static_cast<int>(normalized * plotHeight);
}

void PlotRenderer::findMinMax(const std::vector<double>& data, double& minVal, double& maxVal) const {
    if (data.empty()) {
        minVal = 0.0;
        maxVal = 1.0;
        return;
    }

    minVal = *std::min_element(data.begin(), data.end());
    maxVal = *std::max_element(data.begin(), data.end());

    // CRITICAL: NO padding! Use exact min/max
    if (minVal == maxVal) {
        minVal -= 0.5;
        maxVal += 0.5;
    }
}

Image PlotRenderer::renderLinePlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    Image img(width_, height_);
    img.clear(255, 255, 255);

    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);

    drawAxes(img);
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);

    // Draw line connecting points
    for (size_t i = 1; i < xdata.size(); ++i) {
        int x1 = mapX(xdata[i-1], xMin, xMax);
        int y1 = mapY(ydata[i-1], yMin, yMax);
        int x2 = mapX(xdata[i], xMin, xMax);
        int y2 = mapY(ydata[i], yMin, yMax);
        drawLine(img, x1, y1, x2, y2, 0, 0, 255);
    }

    // Draw points
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawCircle(img, x, y, 3, 255, 0, 0);
    }

    return img;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    Image img(width_, height_);
    img.clear(255, 255, 255);

    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);

    drawAxes(img);
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);

    // Draw points
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawSquare(img, x, y, 6, 0, 200, 0);
    }

    return img;
}

Image PlotRenderer::renderBarPlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    Image img(width_, height_);
    img.clear(255, 255, 255);

    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);

    // For bar plots, Y axis should start at 0
    if (yMin > 0) yMin = 0;

    drawAxes(img);
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);

    // Draw bars
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int yBottom = mapY(0, yMin, yMax);
        int yTop = mapY(ydata[i], yMin, yMax);

        // Draw bar from bottom to top
        drawLine(img, x, yBottom, x, yTop, 100, 150, 255);

        // Draw top cap
        drawSquare(img, x, yTop, 6, 100, 150, 255);
    }

    return img;
}
