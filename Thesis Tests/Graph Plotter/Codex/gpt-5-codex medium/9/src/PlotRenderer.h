#ifndef PLOT_RENDERER_H
#define PLOT_RENDERER_H

#include "Image.h"

#include <string>
#include <vector>

class PlotRenderer {
public:
    PlotRenderer(int width = 800, int height = 600);

    Image render(const std::vector<double>& xs,
                 const std::vector<double>& ys,
                 const std::string& plotType) const;

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

    Range calculateRange(const std::vector<double>& values) const;
    int mapX(double x, Range xRange) const;
    int mapY(double y, Range yRange) const;

    void drawAxes(Image& image, Range xRange, Range yRange) const;
    void drawGrid(Image& image, Range xRange, Range yRange) const;
    void drawAxisLabels(Image& image, Range xRange, Range yRange) const;

    void drawLine(Image& image, int x0, int y0, int x1, int y1,
                  unsigned char r, unsigned char g, unsigned char b) const;
    void drawCircle(Image& image, int cx, int cy, int radius,
                    unsigned char r, unsigned char g, unsigned char b) const;
    void drawSquare(Image& image, int cx, int cy, int size,
                    unsigned char r, unsigned char g, unsigned char b) const;

    void drawDigit(Image& image, int x, int y, char digit,
                   unsigned char r, unsigned char g, unsigned char b) const;
    void drawText(Image& image, int x, int y, const std::string& text,
                  unsigned char r, unsigned char g, unsigned char b) const;

    Image renderLinePlot(const std::vector<double>& xs,
                         const std::vector<double>& ys,
                         Range xRange,
                         Range yRange) const;

    Image renderScatterPlot(const std::vector<double>& xs,
                            const std::vector<double>& ys,
                            Range xRange,
                            Range yRange) const;

    Image renderBarPlot(const std::vector<double>& xs,
                        const std::vector<double>& ys,
                        Range xRange,
                        Range yRange) const;
};

#endif
