#include "PlotRenderer.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width), height_(height),
      marginLeft_(80), marginRight_(40),
      marginTop_(40), marginBottom_(80) {}

Image PlotRenderer::renderLinePlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    if (xdata.empty() || ydata.empty() || xdata.size() != ydata.size()) {
        throw std::runtime_error("Invalid data for line plot");
    }

    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);

    Image img(width_, height_);
    img.clear(255, 255, 255); // White background

    drawAxes(img);
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);

    // Draw line connecting points
    for (size_t i = 1; i < xdata.size(); ++i) {
        int x1 = mapX(xdata[i-1], xMin, xMax);
        int y1 = mapY(ydata[i-1], yMin, yMax);
        int x2 = mapX(xdata[i], xMin, xMax);
        int y2 = mapY(ydata[i], yMin, yMax);

        drawLine(img, x1, y1, x2, y2, 0, 0, 255); // Blue line
    }

    // Draw points
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawCircle(img, x, y, 3, 255, 0, 0); // Red points
    }

    return img;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    if (xdata.empty() || ydata.empty() || xdata.size() != ydata.size()) {
        throw std::runtime_error("Invalid data for scatter plot");
    }

    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);

    Image img(width_, height_);
    img.clear(255, 255, 255); // White background

    drawAxes(img);
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);

    // Draw points as squares
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawSquare(img, x, y, 6, 0, 200, 0); // Green squares
    }

    return img;
}

Image PlotRenderer::renderBarPlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    if (xdata.empty() || ydata.empty() || xdata.size() != ydata.size()) {
        throw std::runtime_error("Invalid data for bar plot");
    }

    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);

    // For bar plot, Y min should be 0
    if (yMin > 0) {
        yMin = 0;
    }

    Image img(width_, height_);
    img.clear(255, 255, 255); // White background

    drawAxes(img);
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);

    // Calculate bar width
    int plotWidth = width_ - marginLeft_ - marginRight_;
    int barWidth = std::max(1, static_cast<int>(plotWidth / (xdata.size() * 2)));

    // Draw bars
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int yBase = mapY(0, yMin, yMax);
        int yTop = mapY(ydata[i], yMin, yMax);

        // Draw from yBase to yTop
        int barHeight = yBase - yTop;
        if (barHeight > 0) {
            for (int dy = 0; dy < barHeight; ++dy) {
                for (int dx = -barWidth/2; dx <= barWidth/2; ++dx) {
                    img.setPixel(x + dx, yTop + dy, 100, 150, 255); // Light blue bars
                }
            }
        }
    }

    return img;
}

void PlotRenderer::drawAxes(Image& img) {
    // Draw X axis (bottom)
    int x1 = marginLeft_;
    int y1 = height_ - marginBottom_;
    int x2 = width_ - marginRight_;
    int y2 = y1;

    drawLine(img, x1, y1, x2, y2, 0, 0, 0); // Black line

    // Draw Y axis (left)
    x1 = marginLeft_;
    y1 = marginTop_;
    x2 = x1;
    y2 = height_ - marginBottom_;

    drawLine(img, x1, y1, x2, y2, 0, 0, 0); // Black line
}

void PlotRenderer::drawGrid(Image& img, double xMin, double xMax, double yMin, double yMax) {
    // Draw vertical grid lines
    int xMinInt = static_cast<int>(std::ceil(xMin));
    int xMaxInt = static_cast<int>(std::floor(xMax));

    for (int xi = xMinInt; xi <= xMaxInt; ++xi) {
        int x = mapX(xi, xMin, xMax);
        int y1 = marginTop_;
        int y2 = height_ - marginBottom_;

        drawLine(img, x, y1, x, y2, 220, 220, 220); // Light gray
    }

    // Draw horizontal grid lines
    int yMinInt = static_cast<int>(std::ceil(yMin));
    int yMaxInt = static_cast<int>(std::floor(yMax));

    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        int x1 = marginLeft_;
        int x2 = width_ - marginRight_;

        drawLine(img, x1, y, x2, y, 220, 220, 220); // Light gray
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

void PlotRenderer::drawLine(Image& img, int x1, int y1, int x2, int y2,
                            unsigned char r, unsigned char g, unsigned char b) {
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
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void PlotRenderer::drawCircle(Image& img, int cx, int cy, int radius,
                              unsigned char r, unsigned char g, unsigned char b) {
    // Midpoint circle algorithm
    int x = 0;
    int y = radius;
    int d = 1 - radius;

    while (x <= y) {
        img.setPixel(cx + x, cy + y, r, g, b);
        img.setPixel(cx - x, cy + y, r, g, b);
        img.setPixel(cx + x, cy - y, r, g, b);
        img.setPixel(cx - x, cy - y, r, g, b);
        img.setPixel(cx + y, cy + x, r, g, b);
        img.setPixel(cx - y, cy + x, r, g, b);
        img.setPixel(cx + y, cy - x, r, g, b);
        img.setPixel(cx - y, cy - x, r, g, b);

        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void PlotRenderer::drawSquare(Image& img, int cx, int cy, int size,
                              unsigned char r, unsigned char g, unsigned char b) {
    int halfSize = size / 2;
    for (int dy = -halfSize; dy <= halfSize; ++dy) {
        for (int dx = -halfSize; dx <= halfSize; ++dx) {
            img.setPixel(cx + dx, cy + dy, r, g, b);
        }
    }
}

void PlotRenderer::drawText(Image& img, int x, int y, const std::string& text,
                            unsigned char r, unsigned char g, unsigned char b) {
    for (char c : text) {
        if (isdigit(c) || c == '-') {
            drawDigit(img, x, y, c, r, g, b);
            x += 6; // Move to next digit position
        }
    }
}

void PlotRenderer::drawDigit(Image& img, int x, int y, char digit,
                             unsigned char r, unsigned char g, unsigned char b) {
    // Simple 3x5 bitmap font for digits 0-9 and minus sign
    const unsigned char font[11][5] = {
        // 0
        {0x7, 0x5, 0x5, 0x5, 0x7},
        // 1
        {0x2, 0x6, 0x2, 0x2, 0x7},
        // 2
        {0x7, 0x1, 0x7, 0x4, 0x7},
        // 3
        {0x7, 0x1, 0x7, 0x1, 0x7},
        // 4
        {0x5, 0x5, 0x7, 0x1, 0x1},
        // 5
        {0x7, 0x4, 0x7, 0x1, 0x7},
        // 6
        {0x7, 0x4, 0x7, 0x5, 0x7},
        // 7
        {0x7, 0x1, 0x1, 0x1, 0x1},
        // 8
        {0x7, 0x5, 0x7, 0x5, 0x7},
        // 9
        {0x7, 0x5, 0x7, 0x1, 0x7},
        // - (minus)
        {0x0, 0x0, 0x7, 0x0, 0x0}
    };

    int index = (digit == '-') ? 10 : (digit - '0');
    if (index < 0 || index > 10) return;

    for (int dy = 0; dy < 5; ++dy) {
        unsigned char row = font[index][dy];
        for (int dx = 0; dx < 3; ++dx) {
            if (row & (1 << (2 - dx))) {
                img.setPixel(x + dx, y + dy, r, g, b);
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
