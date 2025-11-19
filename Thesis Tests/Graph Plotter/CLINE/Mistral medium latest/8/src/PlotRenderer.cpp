#include "PlotRenderer.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width), height_(height),
      marginLeft_(80), marginRight_(40),
      marginTop_(40), marginBottom_(80) {}

Image PlotRenderer::renderLinePlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    Image img(width_, height_);
    img.clear(255, 255, 255);

    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);

    drawAxes(img);
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);

    for (size_t i = 1; i < xdata.size(); ++i) {
        int x1 = mapX(xdata[i-1], xMin, xMax);
        int y1 = mapY(ydata[i-1], yMin, yMax);
        int x2 = mapX(xdata[i], xMin, xMax);
        int y2 = mapY(ydata[i], yMin, yMax);
        drawLine(img, x1, y1, x2, y2, 0, 0, 255); // Blue line
    }

    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawCircle(img, x, y, 3, 255, 0, 0); // Red dots
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

    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawSquare(img, x, y, 6, 0, 200, 0); // Green squares
    }

    return img;
}

Image PlotRenderer::renderBarPlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    Image img(width_, height_);
    img.clear(255, 255, 255);

    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);

    drawAxes(img);
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);

    int plotWidth = width_ - marginLeft_ - marginRight_;
    int barWidth = std::max(1, plotWidth / static_cast<int>(xdata.size()) - 2);

    for (size_t i = 0; i < xdata.size(); ++i) {
        int x1 = mapX(xdata[i], xMin, xMax) - barWidth / 2;
        int x2 = x1 + barWidth;
        int y1 = mapY(0, yMin, yMax); // Base of bar
        int y2 = mapY(ydata[i], yMin, yMax); // Top of bar

        // Draw bar from y1 to y2
        for (int y = y2; y < y1; ++y) {
            for (int x = x1; x < x2; ++x) {
                img.setPixel(x, y, 100, 150, 255); // Light blue
            }
        }
    }

    return img;
}

void PlotRenderer::drawAxes(Image& img) {
    // X-axis
    drawLine(img, marginLeft_, height_ - marginBottom_,
             width_ - marginRight_, height_ - marginBottom_,
             0, 0, 0);

    // Y-axis
    drawLine(img, marginLeft_, marginTop_,
             marginLeft_, height_ - marginBottom_,
             0, 0, 0);
}

void PlotRenderer::drawGrid(Image& img, double xMin, double xMax, double yMin, double yMax) {
    // Vertical grid lines
    int xMinInt = static_cast<int>(std::ceil(xMin));
    int xMaxInt = static_cast<int>(std::floor(xMax));
    for (int xi = xMinInt; xi <= xMaxInt; ++xi) {
        int x = mapX(xi, xMin, xMax);
        drawLine(img, x, marginTop_, x, height_ - marginBottom_, 220, 220, 220);
    }

    // Horizontal grid lines
    int yMinInt = static_cast<int>(std::ceil(yMin));
    int yMaxInt = static_cast<int>(std::floor(yMax));
    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        drawLine(img, marginLeft_, y, width_ - marginRight_, y, 220, 220, 220);
    }
}

void PlotRenderer::drawAxisLabels(Image& img, double xMin, double xMax, double yMin, double yMax) {
    // X-axis labels
    int xMinInt = static_cast<int>(std::ceil(xMin));
    int xMaxInt = static_cast<int>(std::floor(xMax));
    for (int xi = xMinInt; xi <= xMaxInt; ++xi) {
        int x = mapX(xi, xMin, xMax);
        std::string label = std::to_string(xi);
        drawText(img, x - label.length() * 2, height_ - marginBottom_ + 10, label, 0, 0, 0);
    }

    // Y-axis labels
    int yMinInt = static_cast<int>(std::ceil(yMin));
    int yMaxInt = static_cast<int>(std::floor(yMax));
    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        std::string label = std::to_string(yi);
        drawText(img, marginLeft_ - 30, y - 2, label, 0, 0, 0);
    }
}

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
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void PlotRenderer::drawCircle(Image& img, int cx, int cy, int radius, unsigned char r, unsigned char g, unsigned char b) {
    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            if (x*x + y*y <= radius*radius) {
                img.setPixel(cx + x, cy + y, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image& img, int cx, int cy, int size, unsigned char r, unsigned char g, unsigned char b) {
    int half = size / 2;
    for (int x = cx - half; x <= cx + half; ++x) {
        for (int y = cy - half; y <= cy + half; ++y) {
            img.setPixel(x, y, r, g, b);
        }
    }
}

void PlotRenderer::drawText(Image& img, int x, int y, const std::string& text, unsigned char r, unsigned char g, unsigned char b) {
    for (char c : text) {
        if (isdigit(c) || c == '-') {
            drawDigit(img, x, y, c, r, g, b);
            x += 6; // Move to next character position
        }
    }
}

void PlotRenderer::drawDigit(Image& img, int x, int y, char digit, unsigned char r, unsigned char g, unsigned char b) {
    // Simple 3x5 bitmap font for digits 0-9 and '-'
    const unsigned char font[11][5] = {
        {0x7, 0x5, 0x5, 0x5, 0x7}, // 0
        {0x2, 0x6, 0x2, 0x2, 0x7}, // 1
        {0x7, 0x1, 0x7, 0x4, 0x7}, // 2
        {0x7, 0x1, 0x7, 0x1, 0x7}, // 3
        {0x5, 0x5, 0x7, 0x1, 0x1}, // 4
        {0x7, 0x4, 0x7, 0x1, 0x7}, // 5
        {0x7, 0x4, 0x7, 0x5, 0x7}, // 6
        {0x7, 0x1, 0x1, 0x1, 0x1}, // 7
        {0x7, 0x5, 0x7, 0x5, 0x7}, // 8
        {0x7, 0x5, 0x7, 0x1, 0x7}, // 9
        {0x0, 0x0, 0x7, 0x0, 0x0}  // - (minus sign)
    };

    int index = (digit == '-') ? 10 : (digit - '0');
    for (int row = 0; row < 5; ++row) {
        unsigned char pattern = font[index][row];
        for (int col = 0; col < 3; ++col) {
            if (pattern & (1 << (2 - col))) {
                img.setPixel(x + col, y + row, r, g, b);
            }
        }
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
