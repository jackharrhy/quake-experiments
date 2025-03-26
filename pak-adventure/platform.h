#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>
#endif

#include <string>
#include <functional>
#include <vector>

namespace Platform
{
    // File system operations
    std::string openFileDialog(const std::string &title, const std::string &defaultPath);

#ifdef __EMSCRIPTEN__
    // Web-specific file handling
    void setFileSelectedCallback(const std::function<void(const std::string &, const std::vector<uint8_t> &)> &callback);
    void handleFileSelected(const std::string &filename, const emscripten::val &arrayBuffer);
#endif

    // Window operations
    void setWindowSize(int width, int height);

    // Platform-specific initialization
    void init();
    void cleanup();

    // Platform-specific main loop
    void mainLoop(const std::function<void()> &update);
}