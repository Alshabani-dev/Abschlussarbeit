#ifndef PLOT_RENDERER_H
#define PLOT_RENDERER_H

#include "Image.h"

#include <string>
#include <vector>

class PlotRenderer {
public:
    PlotRenderer(int width, int height);

    Image renderLinePlot(const std::vector<double> &xData,
                         const std::vector<double> &yData) const;
    Image renderScatterPlot(const std::vector<double> &xData,
                            const std::vector<double> &yData) const;
    Image renderBarPlot(const std::vector<double> &xData,
                        const std::vector<double> &yData) const;

private:
    struct Range {
        double min;
        double max;
    };

    int width_;
    int height_;
    int marginLeft_ = 70;
    int marginRight_ = 30;
    int marginTop_ = 30;
    int marginBottom_ = 60;

    Range computeRange(const std::vector<double> &values) const;
    int plotWidth() const;
    int plotHeight() const;
    int mapX(double value, const Range &range) const;
    int mapY(double value, const Range &range) const;

    void drawAxes(Image &image) const;
    void drawGrid(Image &image, const Range &xRange, const Range &yRange) const;
    void drawAxisLabels(Image &image, const Range &xRange, const Range &yRange) const;

    void drawLine(Image &image, int x0, int y0, int x1, int y1,
                  unsigned char r, unsigned char g, unsigned char b) const;
    void drawCircle(Image &image, int cx, int cy, int radius,
                    unsigned char r, unsigned char g, unsigned char b) const;
    void drawSquare(Image &image, int cx, int cy, int size,
                    unsigned char r, unsigned char g, unsigned char b) const;

    void drawDigit(Image &image, int x, int y, char digit,
                   unsigned char r, unsigned char g, unsigned char b) const;
    void drawText(Image &image, int x, int y, const std::string &text,
                  unsigned char r, unsigned char g, unsigned char b) const;

    void commonDecorations(Image &image, const Range &xRange, const Range &yRange) const;
};

#endif
