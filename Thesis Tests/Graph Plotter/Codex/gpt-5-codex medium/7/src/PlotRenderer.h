#ifndef PLOT_RENDERER_H
#define PLOT_RENDERER_H

#include <string>
#include <utility>
#include <vector>

#include "Image.h"

class PlotRenderer {
public:
    struct Color {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };

    PlotRenderer(int width, int height);

    Image renderLinePlot(const std::vector<double> &x, const std::vector<double> &y);
    Image renderScatterPlot(const std::vector<double> &x, const std::vector<double> &y);
    Image renderBarPlot(const std::vector<double> &x, const std::vector<double> &y);

private:
    int width_;
    int height_;
    int marginLeft_ = 70;
    int marginRight_ = 30;
    int marginTop_ = 30;
    int marginBottom_ = 80;

    double minX_ = 0.0;
    double maxX_ = 1.0;
    double minY_ = 0.0;
    double maxY_ = 1.0;

    void computeBounds(const std::vector<double> &x, const std::vector<double> &y);
    int mapX(double value) const;
    int mapY(double value) const;

    void drawAxes(Image &image) const;
    void drawGrid(Image &image) const;
    void drawAxisLabels(Image &image) const;

    void drawLine(Image &image, int x0, int y0, int x1, int y1, Color color) const;
    void drawCircle(Image &image, int cx, int cy, int radius, Color color) const;
    void drawSquare(Image &image, int cx, int cy, int size, Color color) const;

    void drawDigit(Image &image, int x, int y, char digit, Color color) const;
    void drawText(Image &image, int x, int y, const std::string &text, Color color) const;

    std::string formatNumber(double value) const;
    void drawBaseline(Image &image) const;
    Image basePlot(const std::vector<double> &x, const std::vector<double> &y);
};

#endif
