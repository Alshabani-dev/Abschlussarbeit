#include "PlotRenderer.h"
#include <algorithm>
#include <cmath>
#include <string>

PlotRenderer::PlotRenderer(int width, int height)
    : width_(width), height_(height),
      marginLeft_(80), marginRight_(40),
      marginTop_(40), marginBottom_(80) {
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

void PlotRenderer::drawLine(Image& img, int x1, int y1, int x2, int y2, 
                            unsigned char r, unsigned char g, unsigned char b) {
    // Bresenham's line algorithm
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int x = x1;
    int y = y1;
    
    while (true) {
        img.setPixel(x, y, r, g, b);
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void PlotRenderer::drawCircle(Image& img, int cx, int cy, int radius, 
                              unsigned char r, unsigned char g, unsigned char b) {
    // Midpoint circle algorithm - filled
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                img.setPixel(cx + x, cy + y, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image& img, int cx, int cy, int size, 
                              unsigned char r, unsigned char g, unsigned char b) {
    int half = size / 2;
    for (int y = cy - half; y <= cy + half; ++y) {
        for (int x = cx - half; x <= cx + half; ++x) {
            img.setPixel(x, y, r, g, b);
        }
    }
}

void PlotRenderer::drawDigit(Image& img, int x, int y, char digit, 
                             unsigned char r, unsigned char g, unsigned char b) {
    // 3x5 bitmap font for digits 0-9 and minus sign
    static const bool font[11][5][3] = {
        // 0
        {{1,1,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,1,1}},
        // 1
        {{0,1,0}, {1,1,0}, {0,1,0}, {0,1,0}, {1,1,1}},
        // 2
        {{1,1,1}, {0,0,1}, {1,1,1}, {1,0,0}, {1,1,1}},
        // 3
        {{1,1,1}, {0,0,1}, {1,1,1}, {0,0,1}, {1,1,1}},
        // 4
        {{1,0,1}, {1,0,1}, {1,1,1}, {0,0,1}, {0,0,1}},
        // 5
        {{1,1,1}, {1,0,0}, {1,1,1}, {0,0,1}, {1,1,1}},
        // 6
        {{1,1,1}, {1,0,0}, {1,1,1}, {1,0,1}, {1,1,1}},
        // 7
        {{1,1,1}, {0,0,1}, {0,0,1}, {0,0,1}, {0,0,1}},
        // 8
        {{1,1,1}, {1,0,1}, {1,1,1}, {1,0,1}, {1,1,1}},
        // 9
        {{1,1,1}, {1,0,1}, {1,1,1}, {0,0,1}, {1,1,1}},
        // - (minus)
        {{0,0,0}, {0,0,0}, {1,1,1}, {0,0,0}, {0,0,0}}
    };
    
    int idx = -1;
    if (digit >= '0' && digit <= '9') {
        idx = digit - '0';
    } else if (digit == '-') {
        idx = 10;
    }
    
    if (idx == -1) return;
    
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (font[idx][row][col]) {
                img.setPixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawText(Image& img, int x, int y, const std::string& text, 
                            unsigned char r, unsigned char g, unsigned char b) {
    int offsetX = 0;
    for (char c : text) {
        if (c >= '0' && c <= '9') {
            drawDigit(img, x + offsetX, y, c, r, g, b);
            offsetX += 4; // 3 pixels + 1 space
        } else if (c == '-') {
            drawDigit(img, x + offsetX, y, c, r, g, b);
            offsetX += 4;
        } else if (c == '.') {
            img.setPixel(x + offsetX + 1, y + 4, r, g, b);
            offsetX += 3;
        } else {
            offsetX += 2; // Space for unknown characters
        }
    }
}

void PlotRenderer::drawAxes(Image& img) {
    // Draw Y-axis (left edge)
    int xAxis = marginLeft_;
    for (int y = marginTop_; y <= height_ - marginBottom_; ++y) {
        img.setPixel(xAxis, y, 0, 0, 0);
    }
    
    // Draw X-axis (bottom edge)
    int yAxis = height_ - marginBottom_;
    for (int x = marginLeft_; x <= width_ - marginRight_; ++x) {
        img.setPixel(x, yAxis, 0, 0, 0);
    }
}

void PlotRenderer::drawGrid(Image& img, double xMin, double xMax, double yMin, double yMax) {
    // Draw grid lines at EACH INTEGER position
    int xMinInt = static_cast<int>(std::ceil(xMin));
    int xMaxInt = static_cast<int>(std::floor(xMax));
    int yMinInt = static_cast<int>(std::ceil(yMin));
    int yMaxInt = static_cast<int>(std::floor(yMax));
    
    // Vertical grid lines at each X integer
    for (int xi = xMinInt; xi <= xMaxInt; ++xi) {
        int x = mapX(xi, xMin, xMax);
        for (int y = marginTop_; y <= height_ - marginBottom_; ++y) {
            img.setPixel(x, y, 220, 220, 220);
        }
    }
    
    // Horizontal grid lines at each Y integer
    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        for (int x = marginLeft_; x <= width_ - marginRight_; ++x) {
            img.setPixel(x, y, 220, 220, 220);
        }
    }
}

void PlotRenderer::drawAxisLabels(Image& img, double xMin, double xMax, double yMin, double yMax) {
    // Draw label at EACH INTEGER position
    int xMinInt = static_cast<int>(std::ceil(xMin));
    int xMaxInt = static_cast<int>(std::floor(xMax));
    int yMinInt = static_cast<int>(std::ceil(yMin));
    int yMaxInt = static_cast<int>(std::floor(yMax));
    
    // X-axis labels
    for (int xi = xMinInt; xi <= xMaxInt; ++xi) {
        int x = mapX(xi, xMin, xMax);
        std::string label = std::to_string(xi);
        drawText(img, x - static_cast<int>(label.length()) * 2, height_ - marginBottom_ + 10, label, 0, 0, 0);
    }
    
    // Y-axis labels
    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        std::string label = std::to_string(yi);
        drawText(img, marginLeft_ - 30, y - 2, label, 0, 0, 0);
    }
}

Image PlotRenderer::renderLinePlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    Image img(width_, height_);
    
    if (xdata.empty() || ydata.empty() || xdata.size() != ydata.size()) {
        return img;
    }
    
    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);
    
    // Draw grid, axes, and labels
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw lines connecting points (blue)
    for (size_t i = 0; i < xdata.size() - 1; ++i) {
        int x1 = mapX(xdata[i], xMin, xMax);
        int y1 = mapY(ydata[i], yMin, yMax);
        int x2 = mapX(xdata[i + 1], xMin, xMax);
        int y2 = mapY(ydata[i + 1], yMin, yMax);
        drawLine(img, x1, y1, x2, y2, 0, 0, 255);
    }
    
    // Draw points (red circles)
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawCircle(img, x, y, 3, 255, 0, 0);
    }
    
    return img;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    Image img(width_, height_);
    
    if (xdata.empty() || ydata.empty() || xdata.size() != ydata.size()) {
        return img;
    }
    
    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);
    
    // Draw grid, axes, and labels
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw points (green squares)
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawSquare(img, x, y, 6, 0, 200, 0);
    }
    
    return img;
}

Image PlotRenderer::renderBarPlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    Image img(width_, height_);
    
    if (xdata.empty() || ydata.empty() || xdata.size() != ydata.size()) {
        return img;
    }
    
    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);
    
    // Ensure Y range includes 0 for bar plots
    if (yMin > 0) yMin = 0;
    if (yMax < 0) yMax = 0;
    
    // Draw grid, axes, and labels
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw bars (light blue)
    int barWidth = 10;
    if (xdata.size() > 1) {
        int plotWidth = width_ - marginLeft_ - marginRight_;
        barWidth = std::max(5, plotWidth / static_cast<int>(xdata.size() * 2));
    }
    
    int yZero = mapY(0, yMin, yMax);
    
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        
        int barLeft = x - barWidth / 2;
        int barRight = x + barWidth / 2;
        int barTop = std::min(y, yZero);
        int barBottom = std::max(y, yZero);
        
        for (int bx = barLeft; bx <= barRight; ++bx) {
            for (int by = barTop; by <= barBottom; ++by) {
                img.setPixel(bx, by, 100, 150, 255);
            }
        }
    }
    
    return img;
}
