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
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        img.setPixel(cx + x, cy + y, r, g, b);
        img.setPixel(cx + y, cy + x, r, g, b);
        img.setPixel(cx - y, cy + x, r, g, b);
        img.setPixel(cx - x, cy + y, r, g, b);
        img.setPixel(cx - x, cy - y, r, g, b);
        img.setPixel(cx - y, cy - x, r, g, b);
        img.setPixel(cx + y, cy - x, r, g, b);
        img.setPixel(cx + x, cy - y, r, g, b);
        
        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
    
    // Fill the circle
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                img.setPixel(cx + x, cy + y, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image& img, int cx, int cy, int size, 
                              unsigned char r, unsigned char g, unsigned char b) {
    int halfSize = size / 2;
    for (int y = cy - halfSize; y <= cy + halfSize; y++) {
        for (int x = cx - halfSize; x <= cx + halfSize; x++) {
            img.setPixel(x, y, r, g, b);
        }
    }
}

void PlotRenderer::drawDigit(Image& img, int x, int y, char digit, 
                             unsigned char r, unsigned char g, unsigned char b) {
    // 3x5 bitmap font for digits 0-9 and minus sign
    // Each digit is represented as a 3x5 grid (column-major)
    static const bool digits[11][5][3] = {
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
        // - (minus sign)
        {{0,0,0}, {0,0,0}, {1,1,1}, {0,0,0}, {0,0,0}}
    };
    
    int digitIndex = -1;
    if (digit >= '0' && digit <= '9') {
        digitIndex = digit - '0';
    } else if (digit == '-') {
        digitIndex = 10;
    }
    
    if (digitIndex >= 0) {
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 3; col++) {
                if (digits[digitIndex][row][col]) {
                    img.setPixel(x + col, y + row, r, g, b);
                }
            }
        }
    }
}

void PlotRenderer::drawText(Image& img, int x, int y, const std::string& text, 
                            unsigned char r, unsigned char g, unsigned char b) {
    int currentX = x;
    for (char c : text) {
        if (c >= '0' && c <= '9') {
            drawDigit(img, currentX, y, c, r, g, b);
            currentX += 4; // 3 pixels + 1 space
        } else if (c == '-') {
            drawDigit(img, currentX, y, c, r, g, b);
            currentX += 4;
        } else if (c == '.') {
            img.setPixel(currentX + 1, y + 4, r, g, b);
            currentX += 3;
        } else {
            currentX += 2; // Space for unknown characters
        }
    }
}

void PlotRenderer::drawAxes(Image& img) {
    // Draw Y-axis (left vertical line)
    for (int y = marginTop_; y <= height_ - marginBottom_; y++) {
        img.setPixel(marginLeft_, y, 0, 0, 0);
    }
    
    // Draw X-axis (bottom horizontal line)
    for (int x = marginLeft_; x <= width_ - marginRight_; x++) {
        img.setPixel(x, height_ - marginBottom_, 0, 0, 0);
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
        for (int y = marginTop_; y <= height_ - marginBottom_; y++) {
            img.setPixel(x, y, 220, 220, 220);
        }
    }
    
    // Horizontal grid lines at each Y integer
    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        for (int x = marginLeft_; x <= width_ - marginRight_; x++) {
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
    
    // Draw background elements
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw blue lines connecting points
    for (size_t i = 0; i < xdata.size() - 1; i++) {
        int x1 = mapX(xdata[i], xMin, xMax);
        int y1 = mapY(ydata[i], yMin, yMax);
        int x2 = mapX(xdata[i + 1], xMin, xMax);
        int y2 = mapY(ydata[i + 1], yMin, yMax);
        drawLine(img, x1, y1, x2, y2, 0, 0, 255); // Blue
    }
    
    // Draw red dots at each point
    for (size_t i = 0; i < xdata.size(); i++) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawCircle(img, x, y, 3, 255, 0, 0); // Red
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
    
    // Draw background elements
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw green squares at each point
    for (size_t i = 0; i < xdata.size(); i++) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawSquare(img, x, y, 6, 0, 200, 0); // Green
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
    
    // Ensure Y=0 is visible if all values are positive
    if (yMin > 0) yMin = 0;
    if (yMax < 0) yMax = 0;
    
    // Draw background elements
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw light blue bars
    int barWidth = std::max(5, (width_ - marginLeft_ - marginRight_) / static_cast<int>(xdata.size()) / 2);
    
    for (size_t i = 0; i < xdata.size(); i++) {
        int x = mapX(xdata[i], xMin, xMax);
        int yTop = mapY(ydata[i], yMin, yMax);
        int yBottom = mapY(0, yMin, yMax);
        
        // Draw bar from 0 to value
        for (int bx = x - barWidth / 2; bx <= x + barWidth / 2; bx++) {
            int startY = std::min(yTop, yBottom);
            int endY = std::max(yTop, yBottom);
            for (int by = startY; by <= endY; by++) {
                img.setPixel(bx, by, 100, 150, 255); // Light blue
            }
        }
    }
    
    return img;
}
