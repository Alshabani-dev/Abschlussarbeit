# Simple C++ Socket Web Server — WAV Streaming (localhost:8080)

**Role:** You are a **senior software developer**.
**Task:** Review and implement this C++ web server project using socket programming.
**Goal:**
  - Modern HTML/CSS/JS frontend served from /public with custom audio player
  - Working WAV audio streaming from data/track.wav with binary-safe IO
  - Track name display, progress slider, real-time time updates

---

## Table of Contents
1.  [Introduction](#introduction)
2.  [Requirements](#requirements)
3.  [Project Structure](#project-structure)
4.  [How It Works](#how-it-works)
5.  [Build & Run](#build--run)
6.  [Nix Shell](#nix-shell)
7.  [Endpoints](#endpoints)
8.  [Frontend UI Implementation](#frontend-ui-implementation)
9.  [Testing](#testing)
10. [Common Issues & Fixes](#common-issues--fixes)
11. [Implementation Notes](#implementation-notes)

---

## Introduction
This project implements a lightweight HTTP server in **C++ (sockets only)** that serves a modern web UI and streams a local **WAV** file on **`localhost:8080`**.

**Features**
- Minimal C++ socket server: `socket()`, `bind()`, `listen()`, `accept()`, `recv()`, `send()`
- Serves static assets from `public/`
- Streams **`data/track.wav`** with correct `Content-Type: audio/wav`
- **Modern UI** with gradient design, animations, glassmorphism effects
- **Custom audio player** with:
  - Track name display (🎼 track.wav)
  - Draggable progress slider for seeking
  - Current time / total duration (MM:SS format)
  - Real-time progress updates as audio plays
  - Play / Pause / Stop controls with emoji icons (▶ ⏸ ⏹)
  - Color-coded status messages (🎵 green=playing, ⏸ orange=paused, ⏹ red=stopped)
- Binary‑safe IO (no corruption on `\0` bytes)

## Requirements
- Linux/Unix or WSL
- **GCC** with C++17 support
- **CMake** 3.10+
- **Nix** for reproducible dev shell

## Project Structure
```
project-root/
├── src/
│   ├── main.cpp          # Entry point with --port parsing
│   ├── Server.cpp        # Socket server + binary-safe streaming
│   └── Server.h          # Server class definition
├── public/
│   ├── index.html        # Modern UI with custom player
│   ├── styles.css        # ~400 lines: gradients, animations
│   └── app.js            # Seeking, time tracking, controls
├── data/
│   └── track.wav         # Your WAV file here
├── CMakeLists.txt        # C++17 build configuration
├── shell.nix             # Nix development environment
└── README.md
```

**Note:** Only 3 C++ source files needed. No separate HttpRequest/HttpResponse/StaticHandler classes required.

## How It Works
- **Root (`/`)** serves the modern UI page with custom player
- **`/audio`** streams the WAV file from `data/track.wav`
- **`/styles.css`** and **`/app.js`** serve static frontend assets
- Server writes HTTP headers, then streams file in 64KB chunks
- UI uses hidden `<audio>` element controlled by custom JavaScript
- Progress slider allows seeking to any position in track
- Time display updates in real-time during playback (60 FPS)

**HTTP headers for streaming**
```
HTTP/1.1 200 OK
Content-Type: audio/wav
Content-Length: <size-in-bytes>
Cache-Control: no-store
Connection: close
```

## Build & Run

**IMPORTANT:** Always use the Nix shell for building and running to ensure a consistent development environment:

```bash
nix-shell shell.nix --run "mkdir -p build && cd build && cmake .. && make && cd .. && ./build/wavserver --port 8080"
```

**Note:** The Nix shell provides all required dependencies (GCC, CMake, etc.) in a reproducible environment. This ensures consistent builds across different systems.

## Nix Shell
```nix
{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  name = "gcc-dev-shell";
  buildInputs = [ pkgs.gcc pkgs.gdb pkgs.pkg-config pkgs.cmake ];
  shellHook = ''
    echo "🔧 Nix Shell started – GCC & CMake ready!"
    gcc --version
    cmake --version
  '';
}
```

## Endpoints
- `GET /` → `public/index.html`
- `GET /app.js`, `GET /styles.css` → static files
- `GET /audio` → streams `data/track.wav`

## Frontend UI Implementation

### HTML Structure (`public/index.html`)
**Required elements:**
- `<h1>🎵 WAV Stream</h1>` with subtitle
- Track info section: `<div class="track-info">` containing 🎼 icon and `<span class="track-name">track.wav</span>`
- Player wrapper with:
  - Hidden audio element: `<audio id="player" src="/audio" preload="metadata">`
  - Range input: `<input type="range" id="progressBar" min="0" max="100" value="0" step="0.1">`
  - Time display: `<span id="currentTime">0:00</span>` and `<span id="duration">0:00</span>`
- Control buttons: `<button id="play">▶ Play</button>`, pause, stop
- Status element: `<p id="status">Ready to play</p>` with dynamic classes
- Wave animation div for decoration

### JavaScript (`public/app.js`)
**Core functions to implement:**
- `formatTime(seconds)` - Convert seconds to "M:SS" format
- `updateProgress()` - Update slider position and current time on `timeupdate` event
- `setStatus(text, className)` - Update status with color class (playing/paused/stopped)

**Event listeners:**
- Progress bar: `input` (for seeking), `change` (apply seek), `mousedown`/`mouseup` (track dragging state)
- Audio events: `loadedmetadata` (set duration), `timeupdate` (update progress), `ended`, `error`, `seeking`, `seeked`
- Buttons: play (with async/await), pause, stop handlers

**Key logic:**
- Use `isSeeking` flag to prevent conflicts between slider drag and audio timeupdate
- Calculate time from slider: `(sliderValue / 100) * player.duration`
- Update slider CSS custom property `--progress` for visual feedback

### CSS (`public/styles.css`)
**Design requirements (~400 lines):**
- CSS custom properties: `--primary: #6366f1`, `--secondary: #8b5cf6`, `--bg-gradient`
- Body: purple/indigo gradient background with animated dot pattern
- Container: white frosted glass card with `backdrop-filter: blur(10px)`, elevated shadow
- Track info: yellow gradient background with pulsing 🎼 icon animation
- Custom range slider:
  - Track: 8px height, rounded, shows progress with gradient
  - Thumb: 18px circle, purple/violet gradient, scales to 1.2x on hover
- Status bar with color-coded classes:
  - `.playing`: green background (#d1fae5), green border (#10b981)
  - `.paused`: yellow background (#fef3c7), orange border (#f59e0b)
  - `.stopped`: red background (#fee2e2), red border (#ef4444)
- Smooth transitions: 0.3s ease on all interactive elements
- Animations: fadeInUp (card entrance), pulse (icon), wave (bottom decoration)
- Responsive: buttons stack vertically on mobile

## Testing
```bash
# Test all endpoints
curl -s -o /dev/null -w "Home: %{http_code}\n" http://localhost:8080/
curl -s -o /dev/null -w "CSS: %{http_code}\n" http://localhost:8080/styles.css
curl -s -o /dev/null -w "JS: %{http_code}\n" http://localhost:8080/app.js
curl -s -o /dev/null -w "Audio: %{http_code}\n" http://localhost:8080/audio

# Verify binary integrity
curl -o /tmp/out.wav http://localhost:8080/audio && diff -q /tmp/out.wav data/track.wav && echo "✓ OK"
```

## Common Issues & Fixes
1. **File not found (404):** Ensure `data/track.wav` exists and server runs from project root
2. **Binary corruption:** Use `std::array<char, N>` for buffers; avoid `std::string` for raw bytes
3. **Truncated send:** Loop on `send()` until all bytes written; handle `EAGAIN/EINTR`
4. **SIGPIPE crash:** Use `signal(SIGPIPE, SIG_IGN)` in constructor and `MSG_NOSIGNAL` in send()
5. **C++20 features:** Use C++17-compatible code (custom `endsWith` lambda instead of `std::string::ends_with`)
6. **Progress slider not updating:** Ensure `timeupdate` listener attached and `isSeeking` flag prevents conflicts
7. **Time shows NaN:** Check for `isNaN()` and `Infinity` in `formatTime()`
8. **Slider doesn't seek:** Verify `input` event updates `player.currentTime`

## Implementation Notes

### Command-Line Argument Parsing (src/main.cpp)

**IMPORTANT:** The server accepts `--port <number>` command-line argument.

```cpp
int main(int argc, char* argv[]) {
    int port = 8080;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = std::atoi(argv[i + 1]);
            break;
        }
    }

    Server server(port);
    server.run();
    return 0;
}
```

**Common mistake:** Using `std::atoi(argv[1])` directly without checking for the `--port` flag will parse `--port` as 0, causing the server to run on port 0 instead of the intended port.

### Server Class (src/Server.h)
```cpp
class Server {
public:
    explicit Server(int port);
    ~Server();
    void run();
private:
    int port_, serverSock_;
    bool running_;
    void setupSocket();
    void handleClient(int clientSock);
    void handleGet(const std::string& path, int clientSock);
    void sendFile(int clientSock, const std::string& filePath, const std::string& mimeType);
    void sendAudio(int clientSock);
    void send404(int clientSock);
    void send500(int clientSock, const std::string& message);
    std::string getMimeType(const std::string& path) const;
    bool sendAll(int sock, const char* data, size_t length);
};
```

### Key Implementation Details (src/Server.cpp)

**1. SIGPIPE handling and socket setup:**
- Constructor: `signal(SIGPIPE, SIG_IGN)`
- Socket options: `SO_REUSEADDR` for quick restart
- Use `MSG_NOSIGNAL` flag in all send() calls

**2. Binary-safe audio streaming:**
- Use `std::array<char, 65536>` for 64KB chunks
- Open with `O_RDONLY`, get size with `fstat()`
- Send headers: `Content-Type: audio/wav`, `Content-Length`, `Cache-Control: no-store`
- Loop: read from file, call `sendAll()` to handle partial writes

**3. Robust send loop:**
```cpp
bool Server::sendAll(int sock, const char* data, size_t length) {
    size_t totalSent = 0;
    while (totalSent < length) {
        ssize_t sent = send(sock, data + totalSent, length - totalSent, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
            return false;
        }
        totalSent += sent;
    }
    return true;
}
```

**4. C++17 compatibility:**
Use lambda for string matching instead of C++20 `ends_with`:
```cpp
auto endsWith = [](const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
};
```

**5. MIME type detection:**
- `.html` → `text/html`
- `.css` → `text/css`
- `.js` → `application/javascript`
- Default → `application/octet-stream`

### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.10)
project(wavserver)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(SOURCES src/main.cpp src/Server.cpp)
add_executable(wavserver ${SOURCES})
target_include_directories(wavserver PRIVATE src)
```

---
**Implementation complete! Modern WAV streaming server with full playback control.**
