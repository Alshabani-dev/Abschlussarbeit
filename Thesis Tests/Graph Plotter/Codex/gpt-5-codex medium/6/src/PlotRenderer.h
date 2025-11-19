#ifndef PLOT_RENDERER_H
#define PLOT_RENDERER_H

#include "Image.h"

#include <string>
#include <vector>

class PlotRenderer {
public:
    PlotRenderer(int width, int height);

    Image renderLinePlot(const std::vector<double>& xValues, const std::vector<double>& yValues);
    Image renderScatterPlot(const std::vector<double>& xValues, const std::vector<double>& yValues);
    Image renderBarPlot(const std::vector<double>& xValues, const std::vector<double>& yValues);

private:
    struct Range {
        double min;
        double max;
    };

    int width_;
    int height_;
    int marginLeft_;
    int marginRight_;
    int marginTop_;
    int marginBottom_;

    Range findMinMax(const std::vector<double>& values) const;
    int mapX(double value, const Range& range) const;
    int mapY(double value, const Range& range) const;

    void drawLine(Image& image, int x0, int y0, int x1, int y1,
                  unsigned char r, unsigned char g, unsigned char b) const;
    void drawCircle(Image& image, int cx, int cy, int radius,
                    unsigned char r, unsigned char g, unsigned char b) const;
    void drawSquare(Image& image, int cx, int cy, int size,
                    unsigned char r, unsigned char g, unsigned char b) const;
    void drawGrid(Image& image, const Range& xRange, const Range& yRange) const;
    void drawAxes(Image& image) const;
    void drawAxisLabels(Image& image, const Range& xRange, const Range& yRange) const;
    void drawText(Image& image, int x, int y, const std::string& text,
                  unsigned char r, unsigned char g, unsigned char b) const;
    bool drawCharacter(Image& image, int x, int y, char c,
                       unsigned char r, unsigned char g, unsigned char b) const;
};

#endif
