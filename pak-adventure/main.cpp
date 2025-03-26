#define GL_SILENCE_DEPRECATION
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <fstream>
#include <optional>
#include <functional>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
#else
#include <zlib.h>
#include <tinyfiledialogs.h>
#endif
#include <filesystem>
#include "platform.h"

struct PakFileEntry
{
    std::string filename;
    uint32_t offset;
    uint32_t size;
};

struct FileTreeNode
{
    std::string name;
    std::vector<FileTreeNode> children;
    std::optional<PakFileEntry> entry;
};

struct PCXImage
{
    int width;
    int height;
    GLuint textureID;
};

namespace PakParser
{
    struct PakHeader
    {
        std::string signature;
        uint32_t dirOffset;
        uint32_t dirLength;
    };

    auto readHeader(std::ifstream &file) -> std::optional<PakHeader>
    {
        char signature[4];
        uint32_t dirOffset, dirLength;

        file.read(signature, 4);
        file.read(reinterpret_cast<char *>(&dirOffset), 4);
        file.read(reinterpret_cast<char *>(&dirLength), 4);

        if (!file || std::string(signature, 4) != "PACK")
        {
            return std::nullopt;
        }
        return PakHeader{std::string(signature, 4), dirOffset, dirLength};
    }

    auto readEntry(std::ifstream &file) -> PakFileEntry
    {
        char name[56];
        uint32_t offset, size;
        file.read(name, 56);
        file.read(reinterpret_cast<char *>(&offset), 4);
        file.read(reinterpret_cast<char *>(&size), 4);
        return {std::string(name), offset, size};
    }

    auto loadPakFile(const std::string &path) -> std::optional<std::vector<PakFileEntry>>
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return std::nullopt;

        auto header = readHeader(file);
        if (!header)
            return std::nullopt;

        file.seekg(header->dirOffset);
        std::vector<PakFileEntry> entries(header->dirLength / 64);

        std::generate(entries.begin(), entries.end(),
                      [&file]()
                      { return readEntry(file); });

        return entries;
    }
}

namespace PCXParser
{
    struct PCXHeader
    {
        uint8_t manufacturer;
        uint8_t version;
        uint8_t encoding;
        uint8_t bitsPerPixel;
        uint16_t xmin, ymin, xmax, ymax;
        uint16_t hres, vres;
        uint8_t palette[48];
        uint8_t colorPlanes;
        uint16_t bytesPerLine;
    };

    auto readHeader(std::ifstream &file) -> std::optional<PCXHeader>
    {
        PCXHeader header;
        file.read(reinterpret_cast<char *>(&header.manufacturer), 1);
        file.read(reinterpret_cast<char *>(&header.version), 1);
        file.read(reinterpret_cast<char *>(&header.encoding), 1);
        file.read(reinterpret_cast<char *>(&header.bitsPerPixel), 1);
        file.read(reinterpret_cast<char *>(&header.xmin), 2);
        file.read(reinterpret_cast<char *>(&header.ymin), 2);
        file.read(reinterpret_cast<char *>(&header.xmax), 2);
        file.read(reinterpret_cast<char *>(&header.ymax), 2);
        file.read(reinterpret_cast<char *>(&header.hres), 2);
        file.read(reinterpret_cast<char *>(&header.vres), 2);
        file.read(reinterpret_cast<char *>(header.palette), 48);
        file.seekg(1, std::ios::cur); // reserved
        file.read(reinterpret_cast<char *>(&header.colorPlanes), 1);
        file.read(reinterpret_cast<char *>(&header.bytesPerLine), 2);

        return file ? std::optional(header) : std::nullopt;
    }

    auto decodeRLE(const std::vector<uint8_t> &raw, size_t size) -> std::vector<uint8_t>
    {
        std::vector<uint8_t> decoded(size);
        size_t src = 0, dst = 0;

        while (dst < size && src < raw.size())
        {
            uint8_t byte = raw[src++];
            if ((byte & 0xC0) == 0xC0)
            {
                uint8_t count = byte & 0x3F;
                byte = raw[src++];
                std::fill_n(decoded.begin() + dst, std::min<size_t>(count, size - dst), byte);
                dst += count;
            }
            else
            {
                decoded[dst++] = byte;
            }
        }
        return decoded;
    }

    auto loadPCX(const std::string &pakPath, const PakFileEntry &entry) -> std::optional<PCXImage>
    {
        std::ifstream file(pakPath, std::ios::binary);
        if (!file.is_open())
            return std::nullopt;

        file.seekg(entry.offset);
        auto header = readHeader(file);
        if (!header)
            return std::nullopt;

        int width = header->xmax - header->xmin + 1;
        int height = header->ymax - header->ymin + 1;

        // Read the raw image data
        std::vector<uint8_t> raw(entry.size - 128);
        file.seekg(entry.offset + 128);
        file.read(reinterpret_cast<char *>(raw.data()), raw.size());

        // Decode RLE data
        auto data = decodeRLE(raw, width * height);

        // Read the palette
        std::vector<uint8_t> palette(768);
        file.seekg(entry.offset + entry.size - 769);
        uint8_t paletteMarker;
        file.read(reinterpret_cast<char *>(&paletteMarker), 1);
        if (paletteMarker == 12)
        {
            file.read(reinterpret_cast<char *>(palette.data()), 768);
        }

        // Convert indexed color to RGBA
        std::vector<uint8_t> rgba(width * height * 4);
        for (int i = 0; i < width * height; i++)
        {
            uint8_t colorIndex = data[i];
            rgba[i * 4 + 0] = palette[colorIndex * 3 + 0]; // R
            rgba[i * 4 + 1] = palette[colorIndex * 3 + 1]; // G
            rgba[i * 4 + 2] = palette[colorIndex * 3 + 2]; // B
            rgba[i * 4 + 3] = 255;                         // A
        }

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        return PCXImage{width, height, textureID};
    }
}

struct PakViewerState
{
    std::vector<PakFileEntry> entries;
    std::vector<PCXImage> loadedImages;   // Store all loaded images
    std::optional<PCXImage> currentImage; // Currently selected single image
    std::string pakPath;
    int selectedEntry = -1;
    bool showFileDialog = false;
    std::string selectedPath;
    float sidebarWidth = 200.0f;
    FileTreeNode fileTree;
    bool gridView = true;      // Whether we're in grid view mode
    std::string currentFolder; // Current folder path being viewed
    float gridScale = 0.5f;    // Scale factor for grid view
};

void buildFileTree(const std::vector<PakFileEntry> &entries, FileTreeNode &root)
{
    root.children.clear();

    for (const auto &entry : entries)
    {
        std::string path = entry.filename;
        FileTreeNode *current = &root;

        size_t pos = 0;
        while ((pos = path.find('/')) != std::string::npos)
        {
            std::string dir = path.substr(0, pos);
            path = path.substr(pos + 1);

            // Find or create directory node
            auto it = std::find_if(current->children.begin(), current->children.end(),
                                   [&dir](const FileTreeNode &node)
                                   { return node.name == dir; });

            if (it == current->children.end())
            {
                current->children.push_back({dir, {}, std::nullopt});
                current = &current->children.back();
            }
            else
            {
                current = &(*it);
            }
        }

        // Add file node
        if (!path.empty())
        {
            current->children.push_back({path, {}, entry});
        }
    }
}

void collectPCXImages(const FileTreeNode &node, const std::string &pakPath, std::vector<PCXImage> &images)
{
    // If this is a file node with a PCX
    if (node.entry && node.name.find(".pcx") != std::string::npos)
    {
        if (auto image = PCXParser::loadPCX(pakPath, *node.entry))
        {
            images.push_back(*image);
        }
    }

    // Recursively process all children
    for (const auto &child : node.children)
    {
        collectPCXImages(child, pakPath, images);
    }
}

void renderFileTreeNode(const FileTreeNode &node, PakViewerState &state)
{
    if (node.children.empty())
    {
        // This is a file
        if (node.name.find(".pcx") != std::string::npos)
        {
            if (ImGui::Selectable(node.name.c_str(), state.selectedEntry != -1 &&
                                                         state.entries[state.selectedEntry].filename == node.entry->filename))
            {
                state.selectedEntry = std::find_if(state.entries.begin(), state.entries.end(),
                                                   [&](const PakFileEntry &e)
                                                   { return e.filename == node.entry->filename; }) -
                                      state.entries.begin();
                state.gridView = false; // Switch to single view when selecting an image
                state.currentImage = PCXParser::loadPCX(state.pakPath, *node.entry);
            }
        }
    }
    else
    {
        // This is a directory
        if (ImGui::TreeNode(node.name.c_str()))
        {
            for (const auto &child : node.children)
            {
                renderFileTreeNode(child, state);
            }
            ImGui::TreePop();
        }

        // Handle folder selection
        if (ImGui::IsItemClicked())
        {
            state.gridView = true;
            state.currentFolder = node.name;
            // Load all PCX images in this folder and its subfolders
            state.loadedImages.clear();
            collectPCXImages(node, state.pakPath, state.loadedImages);
        }
    }
}

std::string openFileDialog()
{
#ifdef __EMSCRIPTEN__
    // For web, we use the Platform abstraction which handles HTML5 file input
    return Platform::openFileDialog("Select PAK File", "");
#else
    const char *filters[] = {"*.pak", "PAK Files"};
    const char *file = tinyfd_openFileDialog(
        "Select PAK File",
        "",
        2,
        filters,
        "PAK Files",
        0);
    return file ? std::string(file) : "";
#endif
}

auto renderUI(PakViewerState &state) -> void
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Quake II PAK Viewer", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoNavFocus);

    if (ImGui::Button("Open PAK File"))
    {
        state.showFileDialog = true;
    }

    if (state.showFileDialog)
    {
        ImGui::OpenPopup("Open PAK File");
        state.showFileDialog = false;
    }

    if (ImGui::BeginPopupModal("Open PAK File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char path[1024] = "";
        ImGui::InputText("File Path", path, IM_ARRAYSIZE(path));

        if (ImGui::Button("Browse"))
        {
            std::string selectedFile = openFileDialog();
            if (!selectedFile.empty())
            {
                strncpy(path, selectedFile.c_str(), IM_ARRAYSIZE(path) - 1);
                path[IM_ARRAYSIZE(path) - 1] = '\0';
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Open"))
        {
            if (std::filesystem::exists(path))
            {
                auto newEntries = PakParser::loadPakFile(path);
                if (newEntries)
                {
                    state.entries = *newEntries;
                    state.pakPath = path;
                    state.currentImage = std::nullopt;
                    state.selectedEntry = -1;
                    buildFileTree(state.entries, state.fileTree);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    const float minSidebarWidth = 100.0f;
    const float maxSidebarWidth = 400.0f;

    ImGui::BeginChild("FileList", ImVec2(state.sidebarWidth, 0), true);
    if (ImGui::TreeNode("PAK Contents"))
    {
        for (const auto &node : state.fileTree.children)
        {
            renderFileTreeNode(node, state);
        }
        ImGui::TreePop();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Use ImGui's native splitter
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

    // Get the window height for the splitter
    float windowHeight = ImGui::GetWindowHeight();
    ImGui::Button("##splitter", ImVec2(4, windowHeight));
    ImGui::PopStyleVar(2);

    if (ImGui::IsItemActive())
    {
        float delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x;
        state.sidebarWidth = std::clamp(state.sidebarWidth + delta, minSidebarWidth, maxSidebarWidth);
        ImGui::ResetMouseDragDelta();
    }

    // Add hover effect to make it more obvious it's draggable
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    ImGui::SameLine();
    ImGui::BeginChild("ImageView", ImVec2(0, 0), true);

    if (state.gridView)
    {
        // Grid view controls
        ImGui::SliderFloat("Grid Scale", &state.gridScale, 0.1f, 2.0f);

        // Calculate grid layout
        float windowWidth = ImGui::GetWindowWidth();
        float windowHeight = ImGui::GetWindowHeight();
        float cellSize = 200.0f * state.gridScale;
        int columns = static_cast<int>(windowWidth / cellSize);
        columns = std::max(1, columns);

        ImGui::BeginGroup();
        for (size_t i = 0; i < state.loadedImages.size(); ++i)
        {
            if (i % columns != 0)
                ImGui::SameLine();

            const auto &image = state.loadedImages[i];
            float scaledWidth = cellSize;
            float scaledHeight = (cellSize * image.height) / image.width;

            ImGui::Image((ImTextureID)(uintptr_t)image.textureID,
                         ImVec2(scaledWidth, scaledHeight));
        }
        ImGui::EndGroup();
    }
    else if (state.currentImage)
    {
        // Single image view
        ImGui::Image((ImTextureID)(uintptr_t)state.currentImage->textureID,
                     ImVec2(state.currentImage->width, state.currentImage->height));
    }

    ImGui::EndChild();

    ImGui::End();
}

int main()
{
    printf("Starting application initialization...\n");
    if (!glfwInit())
    {
        printf("Failed to initialize GLFW\n");
        return -1;
    }
    printf("GLFW initialized successfully\n");

#ifdef __EMSCRIPTEN__
    printf("Running in Emscripten environment\n");
    // For web, we use WebGL2
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

    // Set up file handling callback
    Platform::setFileSelectedCallback([](const std::string &filename, const std::vector<uint8_t> &data)
                                      {
        printf("File received: %s, size: %zu bytes\n", filename.c_str(), data.size());
        
        // Create a temporary file to handle the data
        // This is a workaround until we implement memory-based file reading
        std::string tempFilename = filename;
        FILE* tempFile = fopen(tempFilename.c_str(), "wb");
        if (tempFile) {
            fwrite(data.data(), 1, data.size(), tempFile);
            fclose(tempFile);
            
            // Try to load the PAK file
            auto entries = PakParser::loadPakFile(tempFilename);
            if (entries) {
                printf("PAK file loaded successfully with %zu entries\n", entries->size());
                // Update the UI state here
                // Note: We need to be careful about accessing state from this callback
                // You might want to store the entries in a queue and process them in the main loop
            } else {
                printf("Failed to load PAK file\n");
            }
            
            // Clean up the temporary file
            remove(tempFilename.c_str());
        } else {
            printf("Failed to create temporary file\n");
        } });
#else
    printf("Running in desktop environment\n");
    // Desktop OpenGL 4.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(1280, 720, "Quake II PAK Viewer", nullptr, nullptr);
    if (!window)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    printf("GLFW window created successfully\n");
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    printf("Initializing ImGui...\n");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 410");
#endif
    printf("ImGui initialized successfully\n");

    PakViewerState state;

    printf("Initializing platform...\n");
    Platform::init();
    printf("Platform initialized successfully\n");

    printf("Starting main loop...\n");
    Platform::mainLoop([&]()
                       {
        printf("Main loop iteration\n");
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderUI(state);

        ImGui::Render();
        int display_w, display_h;
#ifdef __EMSCRIPTEN__
        emscripten_get_canvas_element_size("#canvas", &display_w, &display_h);
#else
        glfwGetFramebufferSize(window, &display_w, &display_h);
#endif
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window); });

    printf("Main loop ended, cleaning up...\n");
    // Clean up all loaded textures
    if (state.currentImage)
    {
        glDeleteTextures(1, &state.currentImage->textureID);
    }
    for (const auto &image : state.loadedImages)
    {
        glDeleteTextures(1, &image.textureID);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#ifndef __EMSCRIPTEN__
    glfwDestroyWindow(window);
#endif
    glfwTerminate();

    Platform::cleanup();

    printf("Application cleanup completed\n");
    return 0;
}
