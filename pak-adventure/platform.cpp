#include "platform.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#else
#include <tinyfiledialogs.h>
#endif

namespace Platform
{
#ifdef __EMSCRIPTEN__
        // Callback function to handle file selection
        static std::function<void(const std::string &, const std::vector<uint8_t> &)> fileSelectedCallback;

        void setFileSelectedCallback(const std::function<void(const std::string &, const std::vector<uint8_t> &)> &callback)
        {
                fileSelectedCallback = callback;
        }

        // Function to be called from JavaScript when a file is selected
        void handleFileSelected(const std::string &filename, const emscripten::val &arrayBuffer)
        {
                printf("File selected: %s\n", filename.c_str());

                // Convert ArrayBuffer to vector<uint8_t>
                size_t length = arrayBuffer["byteLength"].as<size_t>();
                std::vector<uint8_t> data(length);
                emscripten::val uint8Array = emscripten::val::global("Uint8Array").new_(arrayBuffer);

                // Copy the data
                emscripten::val memoryView = emscripten::val::module_property("HEAPU8").call<emscripten::val>("subarray",
                                                                                                              reinterpret_cast<uintptr_t>(data.data()),
                                                                                                              reinterpret_cast<uintptr_t>(data.data() + length));
                memoryView.call<void>("set", uint8Array);

                if (fileSelectedCallback)
                {
                        fileSelectedCallback(filename, data);
                }
        }
#endif

        std::string openFileDialog(const std::string &title, const std::string &defaultPath)
        {
#ifdef __EMSCRIPTEN__
                // For web, we'll use HTML5 file input
                EM_ASM({
                        var input = document.createElement('input');
                        input.type = 'file';
                        input.accept = '.pak';
                        input.onchange = function(e)
                        {
                                var file = e.target.files[0];
                                if (!file)
                                        return;

                                var reader = new FileReader();
                                reader.onload = function(e)
                                {
                                        var arrayBuffer = e.target.result;
                                        // Call our C++ callback with the file data
                                        Module.handleFileSelected(file.name, arrayBuffer);
                                };
                                reader.readAsArrayBuffer(file);
                        };
                        input.click();
                });
                return ""; // Actual file handling is done asynchronously
#else
                const char *filters[] = {"*.pak", "PAK Files"};
                const char *file = tinyfd_openFileDialog(
                    title.c_str(),
                    defaultPath.c_str(),
                    2,
                    filters,
                    "PAK Files",
                    0);
                return file ? std::string(file) : "";
#endif
        }

        void setWindowSize(int width, int height)
        {
#ifdef __EMSCRIPTEN__
                auto canvas = emscripten::val::global("document").call<emscripten::val>("getElementById", std::string("canvas"));
                if (!canvas.isNull())
                {
                        canvas.set("width", width);
                        canvas.set("height", height);
                }
#else
                // Desktop implementation would go here if needed
#endif
        }

        void init()
        {
                printf("Platform::init called\n");
#ifdef __EMSCRIPTEN__
                printf("Initializing Emscripten platform\n");
                // Initialize Emscripten-specific features
                auto canvas = emscripten::val::global("document").call<emscripten::val>("getElementById", std::string("canvas"));
                if (!canvas.isNull())
                {
                        printf("Canvas found, setting dimensions\n");
                        canvas.set("width", 1280);
                        canvas.set("height", 720);
                        printf("Canvas dimensions set\n");
                }
                else
                {
                        printf("Canvas not found!\n");
                }
#else
                printf("Initializing desktop platform\n");
#endif
        }

        void cleanup()
        {
                printf("Platform::cleanup called\n");
#ifdef __EMSCRIPTEN__
                printf("Cleaning up Emscripten platform\n");
#else
                printf("Cleaning up desktop platform\n");
#endif
        }

        void mainLoop(const std::function<void()> &update)
        {
                printf("Platform::mainLoop called\n");
#ifdef __EMSCRIPTEN__
                // Store the update function in a static variable to avoid lifetime issues
                static std::function<void()> s_update;
                s_update = update;

                printf("Setting up Emscripten main loop\n");
                emscripten_set_main_loop_arg(
                    [](void *arg)
                    {
                            auto *loop = static_cast<std::function<void()> *>(arg);
                            (*loop)();
                    },
                    &s_update,
                    0, // Use requestAnimationFrame
                    1  // Simulate infinite loop
                );
                printf("Emscripten main loop set up complete\n");
#else
                while (true)
                {
                        update();
                }
#endif
        }

#ifdef __EMSCRIPTEN__
        // Expose functions to JavaScript
        EMSCRIPTEN_BINDINGS(platform)
        {
                emscripten::function("init_web", &init);
                emscripten::function("setWindowSize", &setWindowSize);
                emscripten::function("handleFileSelected", &handleFileSelected);
        }
#endif
}