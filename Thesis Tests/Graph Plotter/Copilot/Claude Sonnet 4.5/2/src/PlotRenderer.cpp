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
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
    
    // Fill circle
    for (int i = -radius; i <= radius; ++i) {
        for (int j = -radius; j <= radius; ++j) {
            if (i*i + j*j <= radius*radius) {
                img.setPixel(cx + i, cy + j, r, g, b);
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
    // Simple 3x5 bitmap font for digits 0-9 and minus sign
    // Each digit is represented as a 3x5 grid (1 = pixel on, 0 = pixel off)
    static const int font[11][5] = {
        // 0
        {0b111, 0b101, 0b101, 0b101, 0b111},
        // 1
        {0b010, 0b110, 0b010, 0b010, 0b111},
        // 2
        {0b111, 0b001, 0b111, 0b100, 0b111},
        // 3
        {0b111, 0b001, 0b111, 0b001, 0b111},
        // 4
        {0b101, 0b101, 0b111, 0b001, 0b001},
        // 5
        {0b111, 0b100, 0b111, 0b001, 0b111},
        // 6
        {0b111, 0b100, 0b111, 0b101, 0b111},
        // 7
        {0b111, 0b001, 0b010, 0b010, 0b010},
        // 8
        {0b111, 0b101, 0b111, 0b101, 0b111},
        // 9
        {0b111, 0b101, 0b111, 0b001, 0b111},
        // - (minus)
        {0b000, 0b000, 0b111, 0b000, 0b000}
    };
    
    int index = -1;
    if (digit >= '0' && digit <= '9') {
        index = digit - '0';
    } else if (digit == '-') {
        index = 10;
    }
    
    if (index >= 0 && index < 11) {
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 3; ++col) {
                if (font[index][row] & (1 << (2 - col))) {
                    img.setPixel(x + col, y + row, r, g, b);
                }
            }
        }
    }
}

void PlotRenderer::drawText(Image& img, int x, int y, const std::string& text,
                           unsigned char r, unsigned char g, unsigned char b) {
    int xOffset = 0;
    for (char c : text) {
        drawDigit(img, x + xOffset, y, c, r, g, b);
        xOffset += 4; // 3 pixels + 1 space
    }
}

void PlotRenderer::drawAxes(Image& img) {
    // Draw Y-axis (left)
    drawLine(img, marginLeft_, marginTop_, marginLeft_, height_ - marginBottom_, 0, 0, 0);
    
    // Draw X-axis (bottom)
    drawLine(img, marginLeft_, height_ - marginBottom_, 
             width_ - marginRight_, height_ - marginBottom_, 0, 0, 0);
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
        drawLine(img, x, marginTop_, x, height_ - marginBottom_, 220, 220, 220);
    }
    
    // Horizontal grid lines at each Y integer
    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        drawLine(img, marginLeft_, y, width_ - marginRight_, y, 220, 220, 220);
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
    img.clear(255, 255, 255); // White background
    
    if (xdata.empty() || ydata.empty() || xdata.size() != ydata.size()) {
        return img;
    }
    
    // Find min/max values
    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);
    
    // Draw grid and axes
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
    
    // Draw points (red dots)
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawCircle(img, x, y, 3, 255, 0, 0);
    }
    
    return img;
}

Image PlotRenderer::renderScatterPlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    Image img(width_, height_);
    img.clear(255, 255, 255); // White background
    
    if (xdata.empty() || ydata.empty() || xdata.size() != ydata.size()) {
        return img;
    }
    
    // Find min/max values
    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);
    
    // Draw grid and axes
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw green squares at each point
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawSquare(img, x, y, 6, 0, 200, 0);
    }
    
    return img;
}

Image PlotRenderer::renderBarPlot(const std::vector<double>& xdata, const std::vector<double>& ydata) {
    Image img(width_, height_);
    img.clear(255, 255, 255); // White background
    
    if (xdata.empty() || ydata.empty() || xdata.size() != ydata.size()) {
        return img;
    }
    
    // Find min/max values
    double xMin, xMax, yMin, yMax;
    findMinMax(xdata, xMin, xMax);
    findMinMax(ydata, yMin, yMax);
    
    // Draw grid and axes
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw bars (light blue)
    int y0 = mapY(0, yMin, yMax); // Y coordinate for value 0
    
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        
        // Draw bar from y0 to y
        int barWidth = 10;
        for (int bx = x - barWidth/2; bx <= x + barWidth/2; ++bx) {
            int yStart = std::min(y0, y);
            int yEnd = std::max(y0, y);
            for (int by = yStart; by <= yEnd; ++by) {
                img.setPixel(bx, by, 100, 150, 255);
            }
        }
    }
    
    return img;
}
