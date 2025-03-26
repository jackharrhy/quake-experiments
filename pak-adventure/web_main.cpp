#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "platform.h"

using namespace emscripten;

// Expose necessary functions to JavaScript
EMSCRIPTEN_BINDINGS(quake_pak_viewer)
{
    function("openFileDialog", &Platform::openFileDialog);
}

// Initialize Emscripten
extern "C"
{
    EMSCRIPTEN_KEEPALIVE
    void init_web()
    {
        // Set up any web-specific initialization here
        EM_ASM(
            var canvas = document.getElementById('canvas');
            canvas.width = 1280;
            canvas.height = 720;);
    }
}
#endif