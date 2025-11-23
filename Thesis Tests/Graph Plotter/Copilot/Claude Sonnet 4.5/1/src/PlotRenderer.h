#ifndef PLOT_RENDERER_H
#define PLOT_RENDERER_H

#include <vector>
#include "Image.h"

class PlotRenderer {
public:
    PlotRenderer(int width = 800, int height = 600);
    
    Image renderLinePlot(const std::vector<double>& xdata, const std::vector<double>& ydata);
    Image renderScatterPlot(const std::vector<double>& xdata, const std::vector<double>& ydata);
    Image renderBarPlot(const std::vector<double>& xdata, const std::vector<double>& ydata);

private:
    int width_;
    int height_;
    int marginLeft_;    // Set to 80
    int marginRight_;   // Set to 40
    int marginTop_;     // Set to 40
    int marginBottom_;  // Set to 80
    
    void drawAxes(Image& img);
    void drawGrid(Image& img, double xMin, double xMax, double yMin, double yMax);
    void drawAxisLabels(Image& img, double xMin, double xMax, double yMin, double yMax);
    void drawLine(Image& img, int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b);
    void drawCircle(Image& img, int cx, int cy, int radius, unsigned char r, unsigned char g, unsigned char b);
    void drawSquare(Image& img, int cx, int cy, int size, unsigned char r, unsigned char g, unsigned char b);
    void drawText(Image& img, int x, int y, const std::string& text, unsigned char r, unsigned char g, unsigned char b);
    void drawDigit(Image& img, int x, int y, char digit, unsigned char r, unsigned char g, unsigned char b);
    
    int mapX(double xValue, double xMin, double xMax) const;
    int mapY(double yValue, double yMin, double yMax) const;
    void findMinMax(const std::vector<double>& data, double& minVal, double& maxVal) const;
};

#endif
