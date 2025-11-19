#ifndef PLOT_RENDERER_H
#define PLOT_RENDERER_H

#include "Image.h"

#include <string>
#include <vector>

enum class PlotType { Line, Scatter, Bar };

class PlotRenderer {
public:
    PlotRenderer(int width = 640, int height = 480);

    Image renderLinePlot(const std::vector<double> &x, const std::vector<double> &y);
    Image renderScatterPlot(const std::vector<double> &x, const std::vector<double> &y);
    Image renderBarPlot(const std::vector<double> &x, const std::vector<double> &y);
    Image renderPlot(const std::vector<double> &x, const std::vector<double> &y, PlotType type);

private:
    struct Range {
        double min = 0.0;
        double max = 1.0;
    };

    struct Color {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };

    int width_;
    int height_;
    int marginLeft_;
    int marginRight_;
    int marginTop_;
    int marginBottom_;

    Range findMinMax(const std::vector<double> &values) const;
    int mapX(double value, const Range &range) const;
    int mapY(double value, const Range &range) const;

    void drawAxes(Image &image, const Range &xRange, const Range &yRange);
    void drawGrid(Image &image, const Range &xRange, const Range &yRange);
    void drawAxisLabels(Image &image, const Range &xRange, const Range &yRange);

    void drawLine(Image &image, int x0, int y0, int x1, int y1, Color color);
    void drawCircle(Image &image, int cx, int cy, int radius, Color color);
    void drawSquare(Image &image, int cx, int cy, int size, Color color);
    void fillRect(Image &image, int x0, int y0, int x1, int y1, Color color);

    void drawDigit(Image &image, int x, int y, char digit, Color color);
    void drawText(Image &image, int x, int y, const std::string &text, Color color);
    int measureTextWidth(const std::string &text) const;

    void renderLineSeries(Image &image, const std::vector<int> &xs, const std::vector<int> &ys, Color color);
    void renderScatterSeries(Image &image, const std::vector<int> &xs, const std::vector<int> &ys, Color color);
    void renderBarSeries(Image &image, const std::vector<int> &xs, const std::vector<int> &ys, double zeroY, Color color);
};

#endif
