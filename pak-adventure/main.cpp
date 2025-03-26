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
#include <zlib.h>
#include <filesystem>
#include <tinyfiledialogs.h>

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
    std::optional<PCXImage> currentImage;
    std::string pakPath;
    int selectedEntry = -1;
    bool showFileDialog = false;
    std::string selectedPath;
    float sidebarWidth = 200.0f;
    FileTreeNode fileTree;
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
    }
}

std::string openFileDialog()
{
    const char *filters[] = {"*.pak", "PAK Files"};
    const char *file = tinyfd_openFileDialog(
        "Select PAK File",
        "",
        2,
        filters,
        "PAK Files",
        0);
    return file ? std::string(file) : "";
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
    if (state.currentImage)
    {
        ImGui::Image((ImTextureID)(uintptr_t)state.currentImage->textureID,
                     ImVec2(state.currentImage->width, state.currentImage->height));
    }
    ImGui::EndChild();

    ImGui::End();
}

int main()
{
    if (!glfwInit())
        return -1;

    // Add OpenGL 4.1 context hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "Quake II PAK Viewer", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    PakViewerState state;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderUI(state);

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    if (state.currentImage)
    {
        glDeleteTextures(1, &state.currentImage->textureID);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
