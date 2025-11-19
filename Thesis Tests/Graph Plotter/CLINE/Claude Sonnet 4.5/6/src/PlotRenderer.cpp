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
    
    // Fill circle
    for (int i = -radius; i <= radius; ++i) {
        for (int j = -radius; j <= radius; ++j) {
            if (i * i + j * j <= radius * radius) {
                img.setPixel(cx + i, cy + j, r, g, b);
            }
        }
    }
}

void PlotRenderer::drawSquare(Image& img, int cx, int cy, int size,
                              unsigned char r, unsigned char g, unsigned char b) {
    int half = size / 2;
    for (int x = cx - half; x <= cx + half; ++x) {
        for (int y = cy - half; y <= cy + half; ++y) {
            img.setPixel(x, y, r, g, b);
        }
    }
}

void PlotRenderer::drawDigit(Image& img, int x, int y, char digit,
                             unsigned char r, unsigned char g, unsigned char b) {
    // 3x5 bitmap font for digits 0-9 and minus sign
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
        // - (minus)
        {{0,0,0}, {0,0,0}, {1,1,1}, {0,0,0}, {0,0,0}}
    };
    
    int index = -1;
    if (digit >= '0' && digit <= '9') {
        index = digit - '0';
    } else if (digit == '-') {
        index = 10;
    }
    
    if (index >= 0) {
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 3; ++col) {
                if (digits[index][row][col]) {
                    img.setPixel(x + col, y + row, r, g, b);
                }
            }
        }
    }
}

void PlotRenderer::drawText(Image& img, int x, int y, const std::string& text,
                            unsigned char r, unsigned char g, unsigned char b) {
    int offset = 0;
    for (char ch : text) {
        drawDigit(img, x + offset, y, ch, r, g, b);
        offset += 4; // 3 pixels wide + 1 pixel spacing
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
    
    // Draw grid, axes, and labels
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw lines connecting points
    for (size_t i = 0; i < xdata.size() - 1; ++i) {
        int x1 = mapX(xdata[i], xMin, xMax);
        int y1 = mapY(ydata[i], yMin, yMax);
        int x2 = mapX(xdata[i + 1], xMin, xMax);
        int y2 = mapY(ydata[i + 1], yMin, yMax);
        
        drawLine(img, x1, y1, x2, y2, 0, 0, 255); // Blue lines
    }
    
    // Draw red dots at each point
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawCircle(img, x, y, 3, 255, 0, 0); // Red dots
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
    
    // Draw grid, axes, and labels
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw green squares at each point
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        drawSquare(img, x, y, 6, 0, 200, 0); // Green squares
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
    
    // Draw grid, axes, and labels
    drawGrid(img, xMin, xMax, yMin, yMax);
    drawAxes(img);
    drawAxisLabels(img, xMin, xMax, yMin, yMax);
    
    // Draw bars from Y=0 to Y=value
    int y0 = mapY(0, yMin, yMax);
    int barWidth = std::max(2, (width_ - marginLeft_ - marginRight_) / static_cast<int>(xdata.size()) / 2);
    
    for (size_t i = 0; i < xdata.size(); ++i) {
        int x = mapX(xdata[i], xMin, xMax);
        int y = mapY(ydata[i], yMin, yMax);
        
        // Draw vertical bar
        for (int bx = x - barWidth; bx <= x + barWidth; ++bx) {
            int startY = std::min(y, y0);
            int endY = std::max(y, y0);
            for (int by = startY; by <= endY; ++by) {
                img.setPixel(bx, by, 100, 150, 255); // Light blue
            }
        }
    }
    
    return img;
}
