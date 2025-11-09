## Project: Simple C++ Web Server

**Role:** You are a **senior software developer**.  
**Task:** Review and implement this C++ web server project using socket programming.  
**Goal:** Build and run the server successfully with HTML/CSS support and working image uploads.


---

### Table of Contents

1.  [Introduction]
2.  [Requirements]
3.  [Project Structure]
4.  [File Templates]
5.  [Architecture Overview]
6.  [Build and Execution]
7.  [Testing & Error Prevention]
8.  [Common Issues and Fixes]
9.  [HTTP Components]
10. [Upload Functionality]
11. [Static File Handling]
12. [Nix Shell Environment]
13. [CSS and HTML Integration]

---

## Introduction

This server implements basic HTTP functionality:

* Receiving and processing HTTP requests via sockets.
* Serving static HTML and CSS files.
* Handling image uploads with validation and saving to the local `Data/` directory.

## Requirements

* Nix Shell (see section "Nix Shell Environment").
* GCC (C++17 or newer).
* CMake.
* Linux/Unix environment.

## Project Structure

```
project-root/
├── src/
│   ├── main.cpp
│   ├── Server.cpp
│   ├── Server.h
│   ├── HttpRequest.cpp
│   ├── HttpRequest.h
│   ├── HttpResponse.cpp
│   ├── HttpResponse.h
│   ├── FileHandler.cpp
│   ├── FileHandler.h
│   ├── UploadHandler.cpp
│   └── UploadHandler.h
├── public/
│   ├── index.html
│   ├── styles.css
│   └── scripts.js
├── Data/
├── build/
├── shell.nix
└── README.md
```

## File Templates

Create header and source skeletons as follows:

```cpp
// Server.h
#ifndef SERVER_H
#define SERVER_H
#include <string>
class Server {
public:
    Server(int port);
    void run();
private:
    int port_;
    void handleClient(int clientSock);
    std::string readRequest(int clientSock);
};
#endif // SERVER_H
```

```cpp
// UploadHandler.h
#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H
#include <string>
class UploadHandler {
public:
    std::string handle(const std::string& body, const std::string& contentType);
private:
    bool isValidImageExtension(const std::string& filename);
};
#endif // UPLOAD_HANDLER_H
```

## Architecture Overview

```
+-------------+
| Client      |
+------+------+
       |
+------+------+
| Server (C++)|<------------------------------+
| - Socket    |                               |
| - Eventloop |---> FileHandler (public/)     |
| - Router    |---> UploadHandler (Data/)     |
+-------------+                               |
                                              |
    +-----------------------------------------+
    | HttpRequest + HttpResponse (binary-safe) |
    +-----------------------------------------+
```

* **Server:** TCP with non-blocking I/O using `socket()`, `bind()`, `listen()`, and `accept()`.
* **Eventloop:** Manages sockets and reads full requests based on `Content-Length`.
* **Router:** Dispatches to endpoints like `/`.
* **HttpRequest:** Separates headers and body, supports both `\r\n\r\n` and `\n\n`.
* **HttpResponse:** Builds status line, headers, and body with correct MIME types.
* **FileHandler:** Serves static files with the right MIME type.
* **UploadHandler:** Robust multipart/form-data handling with flexible boundary detection and binary-safe data handling.

## Build and Execution

Run in one step:

```
nix-shell shell.nix --run "rm -rf build && cmake -S . -B build && cmake --build build && ./build/webserver --port 8080"
```

## Testing & Error Prevention

Use these `curl` commands and check only the HTTP status codes:

```bash
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8080/
curl -s -o /dev/null -w "%{http_code}\n" -F "file=@test.jpg" http://localhost:8080/
curl -s -o /dev/null -w "%{http_code}\n" -F "wrong=@test.jpg" http://localhost:8080/
```

## Common Issues and Fixes

1. **404 Not Found:** Always run from the project root so `public/` is found.
2. **400 Bad Request (Upload):** Do not modify the HTTP body; keep the raw binary stream.
3. **Truncated Requests:** Extract `Content-Length` and read data in a loop until complete.
   - **Known Fix:** Non-blocking sockets deliver payloads in fragments. The server buffers all incoming bytes per client and checks completeness using `Content-Length` before parsing.
4. **Multipart Boundaries:** Support multiple newline styles and dynamic boundary detection.
5. **Invalid File Types:** Check extensions in `isValidImageExtension()` (jpg/jpeg, png, gif, bmp).
6. **Build & Run Integration:** Use a single `nix-shell --run` command to avoid path issues.
7. **Binary Data Corruption:**
   - **Problem:** Uploaded files get corrupted because `std::string` truncates at null bytes (0x00).
   - **Fix:** Use `std::vector<char>` for all HTTP bodies and file contents.
8. **Hanging on Root Page:**
   - **Problem:** Server incorrectly waits for a body on `GET /`.
   - **Fix:** Skip body reading for GET/HEAD requests and include `Content-Length` and `Connection: close` headers.
9. **Incomplete Multipart Data:**
   - **Problem:** Incomplete uploads due to weak boundary detection.
   - **Fix:** Implement flexible boundary recognition using both `\r\n\r\n` and `\n\n`, falling back to data end if boundary missing.
   - **Code Example:**
     ```cpp
     size_t file_data_start = data.find("\r\n\r\n", filename_end);
     if (file_data_start == std::string::npos) {
         file_data_start = data.find("\n\n", filename_end);
         if (file_data_start == std::string::npos) {
             return "400 Bad Request: No file data found";
         }
         file_data_start += 2;
     } else {
         file_data_start += 4;
     }

     size_t file_data_end = data.find(boundary, file_data_start);
     if (file_data_end == std::string::npos) {
         file_data_end = data.length();
     }
     ```
10. **Upload causes Network Error and crashes server:**
    - **Problem:** Browser disconnect triggers `SIGPIPE`, terminating process.
    - **Fix:** Ignore `SIGPIPE` (`std::signal(SIGPIPE, SIG_IGN);`) or use `send()` with `MSG_NOSIGNAL`.

## HTTP Components

* **HttpRequest:** Reads headers up to `\r\n\r\n` and binary-safe body.
* **HttpResponse:** Builds responses with status, headers, and body.
* **Server:** Buffers client data, reads in chunks, and ensures full request before processing.
* **Routing:** Simple if/else logic for endpoints.

## Upload Functionality

* POST `/`
* `Content-Type: multipart/form-data; boundary=...`
* Validation: JPG/JPEG, PNG, GIF, BMP only
* Storage: `Data/<timestamp>_<name>`
* Uses JavaScript popups for success/error messages (no redirect).

## Static File Handling

* `public/` contains `index.html` and `styles.css`.
* Correct MIME type detection and response.

## Nix Shell Environment

```nix
{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "gcc-dev-shell";

  buildInputs = [
    pkgs.gcc
    pkgs.gdb
    pkgs.pkg-config
    pkgs.cmake
  ];

  shellHook = ''
    echo "🔧 Nix Shell started – GCC & CMake ready!"
    gcc --version
    cmake --version
  '';
}
```

## CSS and HTML Integration

* `index.html` links to `<link rel="stylesheet" href="/styles.css">`.
* Responsive layout using Flexbox/Grid.

---

*End of updated README: All known issues addressed and preventive measures included.*
