# Quake II PAK Viewer

A viewer for Quake II PAK files that can run both on desktop and in the web browser.

## Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler
- OpenGL 4.1 or higher
- GLFW3
- ImGui
- tinyfiledialogs
- Emscripten (for web build)

## Building

### Desktop Version

```bash
mkdir build
cd build
cmake ..
make
```

### Web Version

```bash
mkdir build-web
cd build-web
emcmake cmake ..
emmake make
```

## Running

### Desktop Version

```bash
./QuakePakViewer
```

### Web Version

1. Start a local web server in the build directory:

```bash
python3 -m http.server 8000
```

2. Open your web browser and navigate to:

```
http://localhost:8000/QuakePakViewer.html
```

## Features

- View PAK file contents in a tree structure
- Preview PCX images
- Grid view and single image view modes
- Drag-and-drop file loading
- Works on both desktop and web platforms

## License

This project is licensed under the MIT License - see the LICENSE file for details.
