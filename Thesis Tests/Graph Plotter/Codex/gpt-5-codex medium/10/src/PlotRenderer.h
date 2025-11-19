#ifndef PLOT_RENDERER_H
#define PLOT_RENDERER_H

#include "Image.h"

#include <string>
#include <vector>

class PlotRenderer {
public:
    PlotRenderer(int width, int height);

    Image renderLinePlot(const std::vector<double> &xs, const std::vector<double> &ys);
    Image renderScatterPlot(const std::vector<double> &xs, const std::vector<double> &ys);
    Image renderBarPlot(const std::vector<double> &xs, const std::vector<double> &ys);

private:
    int width_;
    int height_;
    int marginLeft_;
    int marginRight_;
    int marginTop_;
    int marginBottom_;

    void findMinMax(const std::vector<double> &data, double &minVal, double &maxVal) const;
    int mapX(double value, double minVal, double maxVal) const;
    int mapY(double value, double minVal, double maxVal) const;

    void drawLine(Image &image, int x0, int y0, int x1, int y1,
                  unsigned char r, unsigned char g, unsigned char b);
    void drawCircle(Image &image, int cx, int cy, int radius,
                    unsigned char r, unsigned char g, unsigned char b);
    void drawSquare(Image &image, int x0, int y0, int x1, int y1,
                    unsigned char r, unsigned char g, unsigned char b);

    void drawDigit(Image &image, int x, int y, char digit,
                   unsigned char r, unsigned char g, unsigned char b);
    void drawText(Image &image, int x, int y, const std::string &text,
                  unsigned char r, unsigned char g, unsigned char b);

    void drawAxes(Image &image, double minX, double maxX, double minY, double maxY);
    void drawGrid(Image &image, double minX, double maxX, double minY, double maxY);
};

#endif
