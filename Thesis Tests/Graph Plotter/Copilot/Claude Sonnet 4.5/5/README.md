# Project: C++ Server-Side Graph Plotting

**Role:** You are a **senior C++ developer**.
**Task:** Implement a web-based graph plotting service in **pure C++ (no external libraries)**.
**Goal:** Build an HTTP server that receives numerical data from a web UI, generates a graph image on the server side, and returns it to the browser.

---

## Table of Contents

1. Introduction
2. Requirements
3. Project Structure
4. File Templates
5. Architecture Overview
6. Build and Execution
7. UI Design
8. Graph Rendering Details
9. HTTP Endpoints
10. Testing & Error Prevention
11. Common Issues and Fixes

---

## Introduction

This project implements a **server-side graph plotting engine** using only C++ and raw sockets. Users interact with a simple web UI where they can:

- Submit numeric data (separate X and Y coordinate arrays)
- Choose plot type (line, scatter, or bar)
- Receive a generated graph image (BMP format)

The server is responsible for:

- Accepting and parsing incoming HTTP POST data
- Generating a pixel buffer representing the graph
- Writing a valid BMP image without external graphics libraries
- Returning the image over HTTP

The focus is on **HTTP handling**, **binary-safe image generation**, and **rendering algorithms**.

---

## Requirements

- C++17 or newer
- CMake 3.10+
- Linux/Unix (uses POSIX sockets)
- **No external libraries** (no graphics libs, no HTTP libs)
- Basic web browser for UI

---

## Project Structure

```text
project-root/
├── src/
│   ├── main.cpp
│   ├── HttpServer.cpp
│   ├── HttpServer.h
│   ├── PlotRenderer.cpp
│   ├── PlotRenderer.h
│   ├── Image.cpp
│   ├── Image.h
│   ├── Utils.h
│   └── Utils.cpp
├── public/
│   ├── index.html
│   └── styles.css
├── build/
├── CMakeLists.txt
└── README.md
```

- **HttpServer**: handles GET/POST requests, routing, and sending images
- **PlotRenderer**: transforms numerical data into a pixel buffer
- **Image**: holds width/height/pixels and exports BMP/PPM images
- **Utils**: helper functions for string manipulation and data parsing
- **public**: static HTML/CSS files served via GET

---

## File Templates

### `Image.h`

```cpp
#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include <string>

class Image {
public:
    Image(int width, int height);

    void setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);
    void clear(unsigned char r, unsigned char g, unsigned char b);
    
    std::string toPPM() const; // Returns binary-safe PPM
    std::string toBMP() const; // Returns BMP encoded data (for browser display)

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    int width_;
    int height_;
    std::vector<unsigned char> pixels_; // 3 bytes per pixel (RGB)
    
    // Helper for BMP format
    void writeInt32(std::vector<unsigned char>& data, int value) const;
    void writeInt16(std::vector<unsigned char>& data, short value) const;
};

#endif
```

**Key Implementation Notes:**
- Use `std::vector<unsigned char>` to avoid binary data corruption
- `toBMP()` creates: 14-byte file header + 40-byte DIB header + pixel data in BGR format
- BMP rows are bottom-to-top and must be padded to 4-byte boundaries
- `toPPM()` is for testing only; browsers need BMP format

### `PlotRenderer.h`

```cpp
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
```

**Key Implementation Notes:**
- Use Bresenham's line algorithm for `drawLine()`
- Implement simple 3x5 bitmap font for digits 0-9 and minus sign in `drawDigit()`
- Grid and labels use integer-aligned positions (loop through `ceil(min)` to `floor(max)`)
- **CRITICAL**: `findMinMax()` must NOT add padding - use exact min/max values

### `HttpServer.h`

```cpp
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <map>

class HttpServer {
public:
    HttpServer(int port);
    ~HttpServer();
    
    void start();
    void stop();

private:
    int port_;
    int serverSocket_;
    bool running_;
    
    void handleClient(int clientSocket);
    std::string readRequest(int clientSocket);
    void parseRequest(const std::string& request, std::string& method, 
                     std::string& path, std::map<std::string, std::string>& headers,
                     std::string& body);
    
    void handleGetRequest(int clientSocket, const std::string& path);
    void handlePostRequest(int clientSocket, const std::string& path, 
                          const std::string& body);
    
    void sendResponse(int clientSocket, const std::string& response);
    void sendResponse(int clientSocket, int statusCode, const std::string& contentType,
                     const std::string& body);
    
    std::string readFile(const std::string& filepath);
    std::string getContentType(const std::string& filepath);
    std::map<std::string, std::string> parseFormData(const std::string& body);
};

#endif
```

**Key Implementation Notes:**
- Use POSIX sockets: `socket()`, `bind()`, `listen()`, `accept()`
- **CRITICAL**: `readRequest()` must handle Content-Length header and read complete POST body
- **CRITICAL**: `parseRequest()` must extract body using `find("\r\n\r\n")` and `substr()`, NOT line-by-line parsing
- `parseFormData()` must URL-decode keys and values
- Try both `public/` and `../public/` paths when serving files (for build directory execution)

### `Utils.h`

```cpp
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace Utils {
    // String manipulation
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string trim(const std::string& str);
    bool startsWith(const std::string& str, const std::string& prefix);
    bool endsWith(const std::string& str, const std::string& suffix);
    
    // Data parsing
    std::vector<double> parseDoubleArray(const std::string& str);
    
    // URL decoding
    std::string urlDecode(const std::string& str);
    
    // Number formatting
    std::string formatNumber(double value, int precision = 1);
}

#endif
```

**Key Implementation Notes:**
- `parseDoubleArray()` splits by comma and uses `std::stod()` with try-catch
- `urlDecode()` handles `%XX` hex encoding and `+` to space conversion

### `index.html` (Web UI Template)

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>C++ Graph Plotting</title>
    <link rel="stylesheet" href="styles.css">
</head>
<body>
    <div class="container">
        <header>
            <h1>C++ Graph Plotting</h1>
            <p class="subtitle">Server-side graph rendering with pure C++</p>
        </header>

        <main>
            <div class="form-section">
                <form id="plotForm">
                    <div class="form-group">
                        <label for="xdata">X Coordinates</label>
                        <textarea id="xdata" name="xdata" placeholder="0,1,2,3,4,5,6,7,8,9" required></textarea>
                        <small>Enter comma-separated X values</small>
                    </div>

                    <div class="form-group">
                        <label for="ydata">Y Coordinates</label>
                        <textarea id="ydata" name="ydata" placeholder="1,4,2,7,6,8,3,9,5,4" required></textarea>
                        <small>Enter comma-separated Y values</small>
                    </div>

                    <div class="form-group">
                        <label for="plotType">Plot Type</label>
                        <!-- CRITICAL: name="plot" NOT "plotType" to match server expectation -->
                        <select id="plotType" name="plot" required>
                            <option value="line">Line Plot</option>
                            <option value="scatter">Scatter Plot</option>
                            <option value="bar">Bar Plot</option>
                        </select>
                    </div>

                    <button type="submit" class="btn-primary">Generate Plot</button>
                </form>
            </div>

            <div class="result-section">
                <div id="imageContainer" style="display: none;">
                    <h3>Generated Plot</h3>
                    <div class="image-wrapper">
                        <img id="plotImage" alt="Generated plot">
                    </div>
                </div>
                <div id="loading" class="loading" style="display: none;">
                    <div class="spinner"></div>
                    <p>Generating plot...</p>
                </div>
                <div id="error" class="error" style="display: none;"></div>
            </div>
        </main>

        <footer>
            <div class="info-box">
                <h3>How to Use</h3>
                <ul>
                    <li>Enter X and Y coordinates as comma-separated values</li>
                    <li>Select a plot type (Line, Scatter, or Bar)</li>
                    <li>Click "Generate Plot" to create your graph</li>
                    <li>View and save your plot image</li>
                </ul>
            </div>
            <div class="info-box">
                <h3>Features</h3>
                <ul>
                    <li>Pure C++ implementation (no external libraries)</li>
                    <li>Server-side image generation in BMP format</li>
                    <li>Integer-aligned axis labels and grid</li>
                    <li>Three plot types: Line, Scatter, and Bar</li>
                </ul>
            </div>
        </footer>
    </div>

    <script>
        document.getElementById('plotForm').addEventListener('submit', async function(e) {
            e.preventDefault();
            
            const xdata = document.getElementById('xdata').value;
            const ydata = document.getElementById('ydata').value;
            const plotType = document.getElementById('plotType').value;
            
            const imageContainer = document.getElementById('imageContainer');
            const loading = document.getElementById('loading');
            const error = document.getElementById('error');
            
            // Hide previous results
            imageContainer.style.display = 'none';
            error.style.display = 'none';
            loading.style.display = 'block';
            
            try {
                const formData = new URLSearchParams();
                formData.append('xdata', xdata);
                formData.append('ydata', ydata);
                formData.append('plot', plotType);
                
                const response = await fetch('/plot', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: formData.toString()
                });
                
                loading.style.display = 'none';
                
                if (!response.ok) {
                    const errorText = await response.text();
                    throw new Error(errorText || 'Failed to generate plot');
                }
                
                // Get image as blob
                const blob = await response.blob();
                const imageUrl = URL.createObjectURL(blob);
                
                // Display image
                const img = document.getElementById('plotImage');
                img.src = imageUrl;
                imageContainer.style.display = 'block';
                
            } catch (err) {
                loading.style.display = 'none';
                error.textContent = 'Error: ' + err.message;
                error.style.display = 'block';
            }
        });
    </script>
</body>
</html>
```

### `styles.css` (UI Styling Template)

```css
/* Base styles */
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    line-height: 1.6;
    color: #1e293b;
    background: linear-gradient(135deg, #dbeafe 0%, #bfdbfe 50%, #93c5fd 100%);
    min-height: 100vh;
    padding: 20px;
}

/* Container and header styles */
.container {
    max-width: 1200px;
    margin: 0 auto;
    background: white;
    border-radius: 20px;
    box-shadow: 0 20px 60px rgba(59, 130, 246, 0.15);
    overflow: hidden;
    border: 2px solid #bfdbfe;
}

header {
    background: linear-gradient(135deg, #3b82f6 0%, #2563eb 50%, #1d4ed8 100%);
    color: white;
    padding: 48px 40px;
    text-align: center;
}

/* Form section styles */
.form-section {
    background: linear-gradient(135deg, #eff6ff 0%, #dbeafe 100%);
    padding: 32px;
    border-radius: 16px;
    margin-bottom: 32px;
    border: 2px solid #bfdbfe;
}

.form-group {
    margin-bottom: 24px;
}

label {
    display: block;
    font-weight: 700;
    margin-bottom: 10px;
    color: #1e40af;
}

textarea,
select {
    width: 100%;
    padding: 14px 16px;
    border: 2px solid #bfdbfe;
    border-radius: 10px;
    font-size: 1em;
    background: white;
    color: #1e293b;
}

/* Button styles */
.btn-primary {
    background: linear-gradient(135deg, #3b82f6 0%, #2563eb 50%, #1d4ed8 100%);
    color: white;
    border: none;
    padding: 16px 40px;
    font-size: 1.1em;
    font-weight: 700;
    border-radius: 12px;
    cursor: pointer;
    width: 100%;
    text-transform: uppercase;
}

/* Responsive design */
@media (max-width: 768px) {
    body {
        padding: 10px;
    }

    header {
        padding: 36px 24px;
    }

    header h1 {
        font-size: 2em;
    }
}

### `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.10)
project(GraphPlotter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Source files
set(SOURCES
    src/main.cpp
    src/HttpServer.cpp
    src/PlotRenderer.cpp
    src/Image.cpp
    src/Utils.cpp
)

# Create executable
add_executable(graphserver ${SOURCES})

# Link threading library (required for sockets on some systems)
find_package(Threads REQUIRED)
target_link_libraries(graphserver Threads::Threads)
```

### `main.cpp`

```cpp
#include "HttpServer.h"
#include <iostream>
#include <cstdlib>
#include <csignal>

HttpServer* globalServer = nullptr;

void signalHandler(int signal) {
    std::cout << "\nShutting down server..." << std::endl;
    if (globalServer) {
        globalServer->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            ++i;
        } else if (arg == "-p" && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            ++i;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --port, -p PORT    Set server port (default: 8080)" << std::endl;
            std::cout << "  --help, -h         Show this help message" << std::endl;
            return 0;
        }
    }
    
    std::cout << "C++ Graph Plotting Server" << std::endl;
    std::cout << "=========================" << std::endl;
    
    HttpServer server(port);
    globalServer = &server;
    
    // Register signal handler for Ctrl+C
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    server.start();
    
    return 0;
}
```

---

## Architecture Overview

```text
+----------------------+
|        Browser       |
|  (HTML/JS Frontend)  |
+-----------+----------+
            |
            | POST /plot
            | xdata=...&ydata=...&plot=line
            v
+-----------+----------+
|       HttpServer     |
| - Serves UI (GET /)  |
| - Parses POST data   |
| - Sends BMP image    |
+-----------+----------+
            |
            | renderLinePlot()
            v
+-----------+----------+
|     PlotRenderer     |
| - Maps coordinates   |
| - Draws axes/grid    |
| - Plots points       |
+-----------+----------+
            |
            | setPixel()
            v
+-----------+----------+
|        Image         |
| - Pixel buffer       |
| - toBMP() encoding   |
+----------------------+
```

The browser sends X and Y coordinate arrays to `/plot`, the server parses them, uses **PlotRenderer** to generate a graph image, and returns the BMP image bytes to the browser.

---

## Build and Execution

### Build

```bash
mkdir build
cd build
cmake ..
make
```

Expected output:
```
[ 16%] Building CXX object CMakeFiles/graphserver.dir/src/main.cpp.o
[ 33%] Building CXX object CMakeFiles/graphserver.dir/src/HttpServer.cpp.o
[ 50%] Building CXX object CMakeFiles/graphserver.dir/src/PlotRenderer.cpp.o
[ 66%] Building CXX object CMakeFiles/graphserver.dir/src/Image.cpp.o
[ 83%] Building CXX object CMakeFiles/graphserver.dir/src/Utils.cpp.o
[100%] Linking CXX executable graphserver
[100%] Built target graphserver
```

### Run the Server

```bash
./graphserver --port 8080
```

Or with custom port:
```bash
./graphserver --port 8081
```

Then open your browser to:
```
http://localhost:8080
```

The UI provides:
- Separate X and Y data entry fields
- Plot type selection (Line, Scatter, Bar)
- Real-time image preview with BMP format
- Loading spinner and error messages
- Modern, clean blue-themed interface

---

## UI Design

The web interface features a modern, professional design with a **cool blue color scheme**:

### Color Palette
- **Background**: Soft blue gradient (#dbeafe → #bfdbfe → #93c5fd)
- **Primary Accent**: Rich blue gradient (#3b82f6 → #2563eb → #1d4ed8)
- **Container**: Clean white with light blue borders (#bfdbfe)
- **Text**: Dark slate (#1e293b) for optimal readability

### Design Features
- **Responsive Layout**: Adapts to mobile, tablet, and desktop screens
- **Form Sections**: Light blue gradient backgrounds (#eff6ff → #dbeafe) with rounded corners
- **Input Fields**: White backgrounds with blue borders that glow on focus
- **Button**: Bold blue gradient with smooth hover animations and shadow effects
- **Image Display**: Framed with blue borders and soft shadows
- **Loading Spinner**: Animated blue spinner with rotating border
- **Error Messages**: Red gradient background (#fee2e2 → #fecaca) with dark red text
- **Custom Scrollbar**: Blue-themed for visual consistency

### User Experience
- Separate input fields for X and Y coordinate data
- Real-time validation and feedback
- Loading spinner during plot generation
- Error messages with clear styling
- Image preview with proper BMP display
- Responsive footer with usage instructions

The design prioritizes usability and visual clarity while maintaining a professional, modern aesthetic.

---

## Graph Rendering Details

### Image Formats

The Image class supports two output formats:

#### BMP (Bitmap) Format - **REQUIRED FOR BROWSERS**
BMP is the **only format that works in browsers**:
- **Native browser support**: All modern browsers display BMP images directly
- **Simple implementation**: No compression, straightforward binary format
- **No external libraries**: Implemented in pure C++
- **24-bit RGB**: Full color support with 8 bits per channel

BMP file structure:
```
[14-byte File Header]
- 'BM' signature (2 bytes)
- File size (4 bytes)
- Reserved (4 bytes)
- Pixel data offset = 54 (4 bytes)

[40-byte DIB Header (BITMAPINFOHEADER)]
- Header size = 40 (4 bytes)
- Width (4 bytes)
- Height (4 bytes)
- Color planes = 1 (2 bytes)
- Bits per pixel = 24 (2 bytes)
- Compression = 0 (4 bytes)
- Image size (4 bytes)
- X pixels per meter (4 bytes)
- Y pixels per meter (4 bytes)
- Colors used (4 bytes)
- Important colors (4 bytes)

[Pixel data]
- BGR format (not RGB!)
- Bottom-to-top row order
- Rows padded to 4-byte boundaries
```

**CRITICAL Implementation Details:**
```cpp
std::string Image::toBMP() const {
    std::vector<unsigned char> bmpData;
    
    // Calculate row padding
    int rowPadding = (4 - (width_ * 3) % 4) % 4;
    int rowSize = width_ * 3 + rowPadding;
    int pixelDataSize = rowSize * height_;
    int fileSize = 54 + pixelDataSize;
    
    // Write headers using little-endian format
    // ... (see implementation)
    
    // Write pixels: bottom-to-top, BGR format
    for (int y = height_ - 1; y >= 0; --y) {
        for (int x = 0; x < width_; ++x) {
            int index = (y * width_ + x) * 3;
            bmpData.push_back(pixels_[index + 2]); // B
            bmpData.push_back(pixels_[index + 1]); // G
            bmpData.push_back(pixels_[index]);     // R
        }
        // Add padding bytes
        for (int p = 0; p < rowPadding; ++p) {
            bmpData.push_back(0);
        }
    }
    
    return std::string(reinterpret_cast<const char*>(bmpData.data()), bmpData.size());
}
```

#### PPM (Portable Pixmap) Format
PPM is available but **NOT for browsers**:
- **Limited browser support**: Browsers don't display PPM
- **Simple format**: Easiest to implement
- **Good for testing**: Works with image viewers like GIMP

Example PPM:
```
P6
800 600
255
[raw RGB bytes]
```

**Important**: Always use BMP format (`toBMP()`) when serving images to browsers.

### XY Coordinate System
The plotting system uses **separate X and Y arrays** for true coordinate plotting:

- **Input**: Two arrays - `xdata` and `ydata` of equal length
- **Mapping**: Each point (xdata[i], ydata[i]) is mapped to pixel coordinates
- **No padding**: Uses exact min/max values from data (**NO 10% padding**)

### Coordinate Mapping Formula

**CRITICAL - Exact implementation:**
```cpp
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
```

### Axis Labels and Grid

**CRITICAL - Integer-aligned implementation:**
```cpp
void PlotRenderer::drawGrid(Image& img, double xMin, double xMax, double yMin, double yMax) {
    // Draw grid lines at EACH INTEGER position
    int xMinInt = static_cast<int>(std::ceil(xMin));
    int xMaxInt = static_cast<int>(std::floor(xMax));
    int yMinInt = static_cast<int>(std::ceil(yMin));
    int yMaxInt = static_cast<int>(std::floor(yMax));
    
    // Vertical grid lines at each X integer
    for (int xi = xMinInt; xi <= xMaxInt; ++xi) {
        int x = mapX(xi, xMin, xMax);
        // Draw vertical line from top to bottom
    }
    
    // Horizontal grid lines at each Y integer
    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        // Draw horizontal line from left to right
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
        drawText(img, x - label.length() * 2, height_ - marginBottom_ + 10, label, 0, 0, 0);
    }
    
    // Y-axis labels
    for (int yi = yMinInt; yi <= yMaxInt; ++yi) {
        int y = mapY(yi, yMin, yMax);
        std::string label = std::to_string(yi);
        drawText(img, marginLeft_ - 30, y - 2, label, 0, 0, 0);
    }
}
```

### Drawing Methods
- **Line plot**: Blue lines (0, 0, 255) connecting consecutive points + red dots (255, 0, 0) at each point (radius 3)
- **Scatter plot**: Green squares (0, 200, 0) at each (X,Y) coordinate (size 6x6)
- **Bar plot**: Light blue bars (100, 150, 255) from Y=0 to Y=value at each X position
- **Axes**: Black lines (0, 0, 0) at left (Y-axis) and bottom (X-axis)
- **Grid**: Light gray lines (220, 220, 220) at each integer position

---

## HTTP Endpoints

### `GET /`
Serves `index.html` with the web UI form

### `GET /styles.css`
Serves the CSS stylesheet

### `POST /plot`
Generates and returns a graph image

**Input format (POST body):**
```
xdata=0,1,2,3,4,5,6,7,8,9&ydata=1,4,2,7,6,8,3,9,5,4&plot=line
```

**Parameters:**
- `xdata`: Comma-separated X coordinates
- `ydata`: Comma-separated Y coordinates (must match length of xdata)
- `plot`: Plot type - `line`, `scatter`, or `bar`

**Server actions:**
1. Parse POST body using `parseFormData()`
2. Extract `xdata`, `ydata`, and `plot` parameters
3. Convert strings to `std::vector<double>` using `Utils::parseDoubleArray()`
4. Validate both arrays have equal length
5. Create `PlotRenderer` and call appropriate render function
6. Call `plotImage.toBMP()` to get BMP data
7. Send HTTP response with `Content-Type: image/bmp`

**Response headers:**
```
HTTP/1.1 200 OK
Content-Type: image/bmp
Content-Length: <size>
Cache-Control: no-store
Connection: close

<BMP binary data>
```

**CRITICAL**: Use `toBMP()` not `toPPM()` for browser display.

---

## Testing & Error Prevention

### Basic Tests

**Test 1: Homepage**
```bash
curl http://localhost:8080/
```
Should return HTML content.

**Test 2: Generate a plot**
```bash
curl -X POST \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "xdata=0,1,2,3,4&ydata=1,2,3,4,5&plot=line" \
  http://localhost:8080/plot \
  --output test.bmp
```

Open `test.bmp` in an image viewer or browser.

**Test 3: All plot types**
```bash
# Line plot
curl -X POST -d "xdata=0,1,2,3,4&ydata=1,4,2,5,3&plot=line" \
  http://localhost:8080/plot --output line.bmp

# Scatter plot
curl -X POST -d "xdata=0,1,2,3,4&ydata=1,4,2,5,3&plot=scatter" \
  http://localhost:8080/plot --output scatter.bmp

# Bar plot
curl -X POST -d "xdata=0,1,2,3,4&ydata=1,4,2,5,3&plot=bar" \
  http://localhost:8080/plot --output bar.bmp
```

### Edge Cases to Test
- Empty data: `xdata=&ydata=&plot=line` → Should return 400 error
- Mismatched lengths: `xdata=1,2,3&ydata=1,2&plot=line` → Should return 400 error
- Single point: `xdata=5&ydata=10&plot=line` → Should work
- Negative values: `xdata=-2,-1,0,1,2&ydata=-5,-2,0,2,5&plot=line` → Should work
- Large values: `xdata=0,100,200&ydata=1000,2000,3000&plot=line` → Should work
- Non-numeric: `xdata=a,b,c&ydata=1,2,3&plot=line` → Should return 400 error
- Invalid plot type: `xdata=1,2&ydata=3,4&plot=invalid` → Should return 400 error

### Validation Checklist
- ✅ Reject empty or missing parameters
- ✅ Reject non-numeric tokens (using try-catch in `parseDoubleArray()`)
- ✅ Reject mismatched array lengths
- ✅ Handle single data point (adjust min/max if equal)
- ✅ Handle negative values correctly
- ✅ Clamp pixel coordinates to image bounds in `setPixel()`

---

## Common Issues and Fixes

### 1. **Binary Data Corruption** ⚠️
   - **Problem**: Using `std::string` for image data may break at `\0` bytes
   - **Root Cause**: String operations can interpret null bytes as terminators
   - **Solution**: Use `std::vector<unsigned char>` for pixel storage, only convert to string at the end
   - **Implementation**:
     ```cpp
     std::vector<unsigned char> pixels_; // Store as vector
     
     // Convert at the end
     return std::string(reinterpret_cast<const char*>(pixels_.data()), pixels_.size());
     ```

### 2. **POST Body Parsing Corruption** ⚠️ **CRITICAL**
   - **Problem**: Form data arrives corrupted, "Invalid plot type" errors
   - **Root Cause**: Using line-by-line parsing (`std::getline`) adds newlines to single-line POST body
   - **Solution**: Extract body directly using string operations:
     ```cpp
     void parseRequest(const std::string& request, ..., std::string& body) {
         // Find body separator
         size_t bodyStart = request.find("\r\n\r\n");
         if (bodyStart != std::string::npos) {
             bodyStart += 4; // Skip past \r\n\r\n
             body = request.substr(bodyStart);
         }
         // ... parse headers normally
     }
     ```
   - **Prevention**: Never parse the body line-by-line

### 3. **Incomplete POST Body** ⚠️
   - **Problem**: Server only reads part of POST data
   - **Root Cause**: Not reading until Content-Length bytes received
   - **Solution**: In `readRequest()`, parse Content-Length header and loop until all bytes received:
     ```cpp
     std::string HttpServer::readRequest(int clientSocket) {
         std::string request;
         char buffer[4096];
         ssize_t bytesRead;

         // Read until we get an empty line (end of headers)
         while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
             request.append(buffer, bytesRead);

             // Check if we have the end of headers
             if (request.find("\r\n\r\n") != std::string::npos) {
                 // Check if this is a POST request with Content-Length
                 size_t contentLengthPos = request.find("Content-Length:");

                 if (contentLengthPos != std::string::npos) {
                     // Extract Content-Length value
                     size_t start = contentLengthPos + 16; // "Content-Length: " is 16 chars
                     size_t end = request.find("\r\n", start);
                     if (end != std::string::npos) {
                         std::string lengthStr = request.substr(start, end - start);
                         int contentLength = std::stoi(Utils::trim(lengthStr));

                         // Calculate how much more we need to read
                         size_t headerEnd = request.find("\r\n\r\n") + 4;
                         size_t bodyRead = request.size() - headerEnd;

                         // Keep reading until we have the full body
                         while (bodyRead < static_cast<size_t>(contentLength)) {
                             bytesRead = recv(clientSocket, buffer,
                                             std::min(sizeof(buffer), static_cast<size_t>(contentLength) - bodyRead), 0);
                             if (bytesRead <= 0) break;
                             request.append(buffer, bytesRead);
                             bodyRead += bytesRead;
                         }
                     }
                 }
                 break;
             }
         }

         return request;
     }
     ```

### 4. **Browser Shows Broken Image** ⚠️ **CRITICAL**
   - **Problem**: Generated image doesn't display in browser
   - **Root Cause**: Sending PPM format instead of BMP
   - **Solution**: 
     1. Implement `toBMP()` method in Image class
     2. Use `plotImage.toBMP()` instead of `plotImage.toPPM()` in HttpServer
     3. Set `Content-Type: image/bmp` header
   - **Why BMP**: All browsers support BMP natively; PPM is not supported
   - **Prevention**: Always test image display in browser, not just file generation

### 5. **Form Field Name Mismatch** ⚠️
   - **Problem**: Server returns "Invalid plot type" even with correct input
   - **Root Cause**: HTML form uses `name="plotType"` but server expects `name="plot"`
   - **Solution**: In HTML, use `<select name="plot">` (NOT `name="plotType"`)
   - **Prevention**: Ensure consistent naming between client and server

### 6. **Coordinate Offset Issues**
   - **Problem**: Points don't appear at correct coordinates
   - **Root Cause**: `findMinMax()` adds 10% padding
   - **Solution**: Remove padding, use exact min/max values
   - **Implementation**: See "Coordinate Mapping Formula" section above

### 7. **Misaligned Grid and Labels**
   - **Problem**: Grid lines don't match axis labels
   - **Root Cause**: Using fixed 6 evenly-spaced lines instead of integer positions
   - **Solution**: Loop through `ceil(min)` to `floor(max)` for integer values
   - **Implementation**: See "Axis Labels and Grid" section above

### 8. **Canvas Appears Flipped**
   - **Problem**: Graph appears upside down
   - **Root Cause**: Pixel (0,0) is top-left, but Y-axis grows downward
   - **Solution**: Invert Y in `mapY()`: `height - marginBottom - (normalized * plotHeight)`

### 9. **File Not Found Errors**
   - **Problem**: Static files (HTML/CSS) return 404
   - **Root Cause**: Server looks for `public/` relative to current directory
   - **Solution**: Try both `public/` and `../public/` paths
   - **Implementation**:
     ```cpp
     std::string content = readFile("public/index.html");
     if (content.empty()) {
         content = readFile("../public/index.html");
     }
     ```

### 10. **Browser Caching Old Images**
   - **Problem**: Browser shows old plot instead of new one
   - **Solution**: Add `Cache-Control: no-store` header to response

### 11. **Port Already in Use**
   - **Problem**: "Failed to bind socket to port 8080"
   - **Solution**: Either kill the process using that port, or use a different port:
     ```bash
     ./graphserver --port 8081
     ```

### 12. **Build Errors**
   - **Problem**: Compilation fails
   - **Common causes**:
     - Missing `#include` statements
     - Forgot to close `#endif` in headers
     - Missing semicolons in class definitions
   - **Solution**: Check compiler error messages carefully, verify all headers are properly closed

---

## Implementation Checklist

When implementing from scratch, follow this order:

### Phase 1: Foundation
- [ ] Create project structure (src/, public/, build/)
- [ ] Write CMakeLists.txt
- [ ] Implement Utils.h/cpp (string operations, URL decode, parse doubles)

### Phase 2: Image Class
- [ ] Implement Image.h header with all methods
- [ ] Implement pixel buffer with `std::vector<unsigned char>`
- [ ] Implement `toBMP()` with proper 14+40 byte headers
- [ ] Implement BGR conversion and row padding
- [ ] Implement `toPPM()` for testing

### Phase 3: Plot Renderer
- [ ] Implement PlotRenderer.h header
- [ ] Implement `findMinMax()` WITHOUT padding
- [ ] Implement `mapX()` and `mapY()` with exact formulas
- [ ] Implement `drawLine()` using Bresenham's algorithm
- [ ] Implement `drawCircle()` and `drawSquare()`
- [ ] Implement `drawDigit()` with 3x5 bitmap font
- [ ] Implement `drawText()` using drawDigit
- [ ] Implement `drawGrid()` with integer-aligned lines
- [ ] Implement `drawAxisLabels()` with integer-aligned labels
- [ ] Implement `drawAxes()`
- [ ] Implement `renderLinePlot()`, `renderScatterPlot()`, `renderBarPlot()`

### Phase 4: HTTP Server
- [ ] Implement HttpServer.h header
- [ ] Implement socket creation, binding, listening (POSIX sockets)
- [ ] Implement `readRequest()` with Content-Length handling
- [ ] Implement `parseRequest()` with direct body extraction
- [ ] Implement `parseFormData()` with URL decoding
- [ ] Implement `handleGetRequest()` with dual path checking
- [ ] Implement `handlePostRequest()` with all three plot types
- [ ] Implement `sendResponse()` for binary-safe transmission
- [ ] Implement `getContentType()` for HTML/CSS/BMP

### Phase 5: Web UI
- [ ] Create index.html with proper form (name="plot"!)
- [ ] Add JavaScript for form submission and image display
- [ ] Create styles.css with blue theme
- [ ] Add loading spinner and error handling

### Phase 6: Main Entry Point
- [ ] Implement main.cpp with argument parsing
- [ ] Add signal handlers for graceful shutdown

### Phase 7: Testing
- [ ] Build with CMake
- [ ] Test GET / (homepage loads)
- [ ] Test GET /styles.css (CSS loads)
- [ ] Test POST /plot with line plot
- [ ] Test POST /plot with scatter plot
- [ ] Test POST /plot with bar plot
- [ ] Test edge cases (see Testing section)
- [ ] Verify BMP images display in browser

---

*End of README*
