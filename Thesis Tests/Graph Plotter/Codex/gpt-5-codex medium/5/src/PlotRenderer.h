#ifndef PLOT_RENDERER_H
#define PLOT_RENDERER_H

#include <string>
#include <vector>

#include "Image.h"

class PlotRenderer {
public:
    enum class PlotType { Line, Scatter, Bar };

    PlotRenderer(int width, int height);

    Image renderPlot(const std::vector<double>& xs,
                     const std::vector<double>& ys,
                     PlotType type);

    static PlotType plotTypeFromString(const std::string& name);

private:
    struct Bounds {
        double minX;
        double maxX;
        double minY;
        double maxY;
    };

    int width_;
    int height_;
    int marginLeft_;
    int marginRight_;
    int marginTop_;
    int marginBottom_;

    Bounds calculateBounds(const std::vector<double>& xs, const std::vector<double>& ys) const;
    double mapX(double value, const Bounds& bounds) const;
    double mapY(double value, const Bounds& bounds) const;

    void drawAxes(Image& image, const Bounds& bounds) const;
    void drawGridAndLabels(Image& image, const Bounds& bounds) const;
    void drawLine(Image& image, int x0, int y0, int x1, int y1,
                  unsigned char r, unsigned char g, unsigned char b) const;
    void drawCircle(Image& image, int cx, int cy, int radius,
                    unsigned char r, unsigned char g, unsigned char b) const;
    void drawRectangle(Image& image, int x0, int y0, int x1, int y1,
                       unsigned char r, unsigned char g, unsigned char b) const;
    void drawText(Image& image, int x, int y, const std::string& text,
                  unsigned char r, unsigned char g, unsigned char b) const;
    void drawDigit(Image& image, int x, int y, char digit,
                   unsigned char r, unsigned char g, unsigned char b) const;

    Image renderLinePlot(const std::vector<double>& xs,
                         const std::vector<double>& ys,
                         const Bounds& bounds);
    Image renderScatterPlot(const std::vector<double>& xs,
                            const std::vector<double>& ys,
                            const Bounds& bounds);
    Image renderBarPlot(const std::vector<double>& xs,
                        const std::vector<double>& ys,
                        const Bounds& bounds);
};

#endif
