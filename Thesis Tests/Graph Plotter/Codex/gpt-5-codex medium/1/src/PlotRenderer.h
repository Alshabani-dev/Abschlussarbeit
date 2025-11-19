#ifndef PLOT_RENDERER_H
#define PLOT_RENDERER_H

#include "Image.h"

#include <string>
#include <vector>

enum class PlotType {
    Line,
    Scatter,
    Bar
};

struct PlotRequest {
    std::vector<double> xs;
    std::vector<double> ys;
    PlotType type;
};

class PlotRenderer {
public:
    PlotRenderer(int width, int height);

    Image render(const PlotRequest& request);

private:
    int width_;
    int height_;

    struct Range {
        double min;
        double max;
    };

    Range findRange(const std::vector<double>& values) const;
    int mapX(double value, const Range& range) const;
    int mapY(double value, const Range& range) const;

    void drawAxes(Image& image, const Range& xRange, const Range& yRange) const;
    void drawGrid(Image& image, const Range& xRange, const Range& yRange) const;
    void drawLabels(Image& image, const Range& xRange, const Range& yRange) const;

    void drawLinePlot(Image& image, const PlotRequest& request, const Range& xRange, const Range& yRange) const;
    void drawScatterPlot(Image& image, const PlotRequest& request, const Range& xRange, const Range& yRange) const;
    void drawBarPlot(Image& image, const PlotRequest& request, const Range& xRange, const Range& yRange) const;

    void drawLine(Image& image, int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b) const;
    void drawCircle(Image& image, int cx, int cy, int radius, unsigned char r, unsigned char g, unsigned char b) const;
    void drawRect(Image& image, int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b) const;
    void drawDigit(Image& image, int x, int y, int digit, unsigned char r, unsigned char g, unsigned char b) const;
    void drawText(Image& image, int x, int y, const std::string& text, unsigned char r, unsigned char g, unsigned char b) const;
};

#endif
