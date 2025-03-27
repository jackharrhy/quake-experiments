#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
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
#include <zip.h>
#include <iostream>
#include <stb_image.h>
#include <unordered_map>
#include <limits>
#include <misc/cpp/imgui_stdlib.h>

enum class PakFormat
{
    PAK,   // Original Quake/Quake 2 .pak format
    PKZIP, // ZIP-based formats (PK3, PK4, etc.)
    UNKNOWN
};

struct PakFileEntry
{
    std::string filename;
    uint32_t offset;
    uint32_t size;
    PakFormat format;
    zip_file_t *zipFile;
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
    std::string filename;
};

struct TextFile
{
    std::string contents;
};

struct BinaryFile
{
    std::vector<uint8_t> data;
};

namespace ParserRegistry
{
    using LoadArchiveFunc = std::optional<std::vector<PakFileEntry>> (*)(const std::string &);
    using ReadDataFunc = std::vector<uint8_t> (*)(const std::string &, const PakFileEntry &);

    struct FormatHandlers
    {
        LoadArchiveFunc loadArchive;
        ReadDataFunc readData;
        std::string description;
    };

    extern std::unordered_map<PakFormat, FormatHandlers> handlers;

    PakFormat getFormatFromExtension(const std::string &extension)
    {
        if (extension == ".pak")
            return PakFormat::PAK;
        if (extension == ".pk3" || extension == ".pk4")
            return PakFormat::PKZIP;
        return PakFormat::UNKNOWN;
    }
}

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

    auto loadArchive(const std::string &path) -> std::optional<std::vector<PakFileEntry>>
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return std::nullopt;

        auto header = readHeader(file);
        if (!header)
            return std::nullopt;

        file.seekg(header->dirOffset);
        std::vector<PakFileEntry> entries(header->dirLength / 64);

        for (auto &entry : entries)
        {
            entry = readEntry(file);
            entry.format = PakFormat::PAK;
        }

        return entries;
    }

    auto readData(const std::string &path, const PakFileEntry &entry) -> std::vector<uint8_t>
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return {};

        file.seekg(entry.offset);
        std::vector<uint8_t> data(entry.size);
        file.read(reinterpret_cast<char *>(data.data()), entry.size);

        return data;
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

        return PCXImage{width, height, textureID, ""}; // Initialize filename field
    }
}

namespace WALParser
{
    struct WALHeader
    {
        char name[32];
        uint32_t width;
        uint32_t height;
        uint32_t offset[4]; // mipmap offsets
        char animname[32];
        uint32_t flags;
        uint32_t contents;
        uint32_t value;
    };

    static std::optional<std::vector<uint8_t>> globalPalette;

    auto loadGlobalPalette(const std::string &pakPath, const std::vector<PakFileEntry> &entries) -> bool
    {
        // Find the colormap.pcx entry
        auto it = std::find_if(entries.begin(), entries.end(),
                               [](const PakFileEntry &e)
                               { return e.filename == "pics/colormap.pcx"; });

        if (it == entries.end())
        {
            return false;
        }

        // Load the PCX file
        std::ifstream file(pakPath, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        file.seekg(it->offset);
        auto pcxImage = PCXParser::loadPCX(pakPath, *it);
        if (!pcxImage)
        {
            return false;
        }

        // Read back the texture data to get the palette
        std::vector<uint8_t> pixels(pcxImage->width * pcxImage->height * 4);
        glBindTexture(GL_TEXTURE_2D, pcxImage->textureID);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        // Convert RGBA to RGB palette
        globalPalette = std::vector<uint8_t>(256 * 3);
        for (int i = 0; i < 256; i++)
        {
            globalPalette->at(i * 3 + 0) = pixels[i * 4 + 0];
            globalPalette->at(i * 3 + 1) = pixels[i * 4 + 1];
            globalPalette->at(i * 3 + 2) = pixels[i * 4 + 2];
        }

        // Clean up the temporary texture
        glDeleteTextures(1, &pcxImage->textureID);
        return true;
    }

    auto readHeader(std::ifstream &file) -> std::optional<WALHeader>
    {
        WALHeader header;
        file.read(header.name, 32);
        file.read(reinterpret_cast<char *>(&header.width), 4);
        file.read(reinterpret_cast<char *>(&header.height), 4);
        file.read(reinterpret_cast<char *>(&header.offset), 16); // 4 mipmap offsets
        file.read(header.animname, 32);
        file.read(reinterpret_cast<char *>(&header.flags), 4);
        file.read(reinterpret_cast<char *>(&header.contents), 4);
        file.read(reinterpret_cast<char *>(&header.value), 4);

        return file ? std::optional(header) : std::nullopt;
    }

    auto loadWAL(const std::string &pakPath, const PakFileEntry &entry) -> std::optional<PCXImage>
    {
        // Load the global palette if we haven't already
        if (!globalPalette)
        {
            // We need to find all entries to locate the colormap
            auto entries = PakParser::loadArchive(pakPath);
            if (!entries || !loadGlobalPalette(pakPath, *entries))
            {
                return std::nullopt;
            }
        }

        std::ifstream file(pakPath, std::ios::binary);
        if (!file.is_open())
            return std::nullopt;

        file.seekg(entry.offset);
        auto header = readHeader(file);
        if (!header)
            return std::nullopt;

        // Read the main image data
        std::vector<uint8_t> data(header->width * header->height);
        file.seekg(entry.offset + header->offset[0]); // First mipmap level
        file.read(reinterpret_cast<char *>(data.data()), data.size());

        // Convert indexed color to RGBA using global palette
        std::vector<uint8_t> rgba(header->width * header->height * 4);
        for (int i = 0; i < header->width * header->height; i++)
        {
            uint8_t colorIndex = data[i];
            // Handle transparent pixels (index 255 is transparent)
            if (colorIndex == 255)
            {
                rgba[i * 4 + 0] = 0; // R
                rgba[i * 4 + 1] = 0; // G
                rgba[i * 4 + 2] = 0; // B
                rgba[i * 4 + 3] = 0; // A (transparent)
            }
            else
            {
                rgba[i * 4 + 0] = globalPalette->at(colorIndex * 3 + 0); // R
                rgba[i * 4 + 1] = globalPalette->at(colorIndex * 3 + 1); // G
                rgba[i * 4 + 2] = globalPalette->at(colorIndex * 3 + 2); // B
                rgba[i * 4 + 3] = 255;                                   // A (opaque)
            }
        }

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, header->width, header->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        return PCXImage{static_cast<int>(header->width), static_cast<int>(header->height), textureID, ""};
    }
}

namespace PKZipParser
{
    auto loadArchive(const std::string &path) -> std::optional<std::vector<PakFileEntry>>
    {
        int error;
        zip_t *archive = zip_open(path.c_str(), 0, &error);
        if (!archive)
            return std::nullopt;

        std::vector<PakFileEntry> entries;
        zip_int64_t num_entries = zip_get_num_entries(archive, 0);

        for (zip_int64_t i = 0; i < num_entries; i++)
        {
            struct zip_stat st;
            if (zip_stat_index(archive, i, 0, &st) == -1)
                continue;

            // Skip directories
            if (st.name[strlen(st.name) - 1] == '/')
                continue;

            PakFileEntry entry;
            entry.filename = st.name;
            entry.size = st.size;
            entry.format = PakFormat::PKZIP;
            entry.zipFile = nullptr; // Will be set when file is opened
            entries.push_back(entry);
        }

        return entries;
    }

    auto readData(const std::string &path, const PakFileEntry &entry) -> std::vector<uint8_t>
    {
        int error;
        zip_t *archive = zip_open(path.c_str(), 0, &error);
        if (!archive)
            return {};

        zip_file_t *file = zip_fopen(archive, entry.filename.c_str(), 0);
        if (!file)
        {
            zip_close(archive);
            return {};
        }

        std::vector<uint8_t> data(entry.size);
        zip_fread(file, data.data(), entry.size);
        zip_fclose(file);
        zip_close(archive);

        return data;
    }
}

namespace TextFileParser
{
    auto loadTextFile(const std::string &pakPath, const PakFileEntry &entry) -> std::optional<TextFile>
    {
        auto readDataFunc = ParserRegistry::handlers[entry.format].readData;
        auto data = readDataFunc(pakPath, entry);

        if (data.empty())
            return std::nullopt;

        return TextFile{std::string(data.begin(), data.end())};
    }
}

namespace BinaryFileParser
{
    auto loadBinaryFile(const std::string &pakPath, const PakFileEntry &entry) -> std::optional<BinaryFile>
    {
        auto readDataFunc = ParserRegistry::handlers[entry.format].readData;
        auto data = readDataFunc(pakPath, entry);

        if (data.empty())
            return std::nullopt;

        return BinaryFile{data};
    }
}

namespace STBImageParser
{
    auto loadSTBImage(const std::string &pakPath, const PakFileEntry &entry) -> std::optional<PCXImage>
    {
        auto readDataFunc = ParserRegistry::handlers[entry.format].readData;
        auto data = readDataFunc(pakPath, entry);

        if (data.empty())
            return std::nullopt;

        int width, height, channels;
        unsigned char *imageData = stbi_load_from_memory(data.data(), data.size(), &width, &height, &channels, STBI_rgb_alpha);
        if (!imageData)
            return std::nullopt;

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(imageData);
        return PCXImage{width, height, textureID, ""};
    }
}

namespace ParserRegistry
{
    std::unordered_map<PakFormat, FormatHandlers> handlers = {
        {PakFormat::PAK, {&PakParser::loadArchive, &PakParser::readData, "Quake/Quake 2 PAK Format"}},
        {PakFormat::PKZIP, {&PKZipParser::loadArchive, &PKZipParser::readData, "ZIP-based Format (PK3/PK4)"}}};
}

struct PakViewerState
{
    std::vector<PakFileEntry> entries;
    std::vector<PCXImage> loadedImages;
    std::optional<PCXImage> currentImage;
    std::optional<TextFile> currentText;
    std::optional<BinaryFile> currentBinary;
    std::string pakPath;
    int selectedEntry = -1;
    bool showFileDialog = false;
    std::string selectedPath;
    float sidebarWidth = 200.0f;
    FileTreeNode fileTree;
    bool gridView = true;
    std::string currentFolder;
    float gridScale = 0.5f;
    std::string searchFilter;
    std::string statusMessage;
};

void setStatusMessage(PakViewerState &state, const std::string &message)
{
    state.statusMessage = message;
    std::cout << "Status: " << message << std::endl;
}

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

        if (!path.empty())
        {
            current->children.push_back({path, {}, entry});
        }
    }
}

void collectSupportedImages(const FileTreeNode &node, const std::string &pakPath, std::vector<PCXImage> &images)
{
    if (node.entry)
    {
        std::string ext = std::filesystem::path(node.name).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".pcx")
        {
            if (auto image = PCXParser::loadPCX(pakPath, *node.entry))
            {
                image->filename = node.entry->filename; // Store the filename
                images.push_back(*image);
            }
        }
        else if (ext == ".wal")
        {
            if (auto image = WALParser::loadWAL(pakPath, *node.entry))
            {
                image->filename = node.entry->filename; // Store the filename
                images.push_back(*image);
            }
        }
        else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga")
        {
            if (auto image = STBImageParser::loadSTBImage(pakPath, *node.entry))
            {
                image->filename = node.entry->filename; // Store the filename
                images.push_back(*image);
            }
        }
    }

    // Recursively process all children
    for (const auto &child : node.children)
    {
        collectSupportedImages(child, pakPath, images);
    }
}

bool stringContainsFilter(const std::string &str, const std::string &filter)
{
    if (filter.empty())
        return true;

    std::string lowerStr = str;
    std::string lowerFilter = filter;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

    return lowerStr.find(lowerFilter) != std::string::npos;
}

bool nodeMatchesFilter(const FileTreeNode &node, const std::string &filter)
{
    if (filter.empty())
        return true;

    if (stringContainsFilter(node.name, filter))
        return true;

    if (node.entry && stringContainsFilter(node.entry->filename, filter))
        return true;

    return false;
}

std::vector<const FileTreeNode *> getFilteredFiles(const FileTreeNode &node, const std::string &filter, int maxResults = std::numeric_limits<int>::max())
{
    std::vector<const FileTreeNode *> results;

    std::vector<const FileTreeNode *> stack;
    stack.push_back(&node);

    while (!stack.empty())
    {
        const FileTreeNode *current = stack.back();
        stack.pop_back();

        if (current->entry)
        {
            if (filter.empty() || stringContainsFilter(current->entry->filename, filter))
            {
                std::string ext = std::filesystem::path(current->name).extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (ext == ".pcx" || ext == ".wal" || ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga")
                {
                    results.push_back(current);
                }
            }
        }
        else
        {
            for (auto it = current->children.rbegin(); it != current->children.rend(); ++it)
            {
                stack.push_back(&(*it));
            }
        }
    }

    return results;
}

void loadFilteredImages(const std::vector<const FileTreeNode *> &filteredNodes, const std::string &pakPath, std::vector<PCXImage> &images)
{
    for (const FileTreeNode *node : filteredNodes)
    {
        if (!node->entry)
            continue;

        std::string ext = std::filesystem::path(node->name).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".pcx")
        {
            if (auto image = PCXParser::loadPCX(pakPath, *node->entry))
            {
                image->filename = node->entry->filename;
                images.push_back(*image);
            }
        }
        else if (ext == ".wal")
        {
            if (auto image = WALParser::loadWAL(pakPath, *node->entry))
            {
                image->filename = node->entry->filename;
                images.push_back(*image);
            }
        }
        else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga")
        {
            if (auto image = STBImageParser::loadSTBImage(pakPath, *node->entry))
            {
                image->filename = node->entry->filename;
                images.push_back(*image);
            }
        }
    }
}

// Helper to check if any children match the filter
bool anyChildrenMatchFilter(const FileTreeNode &node, const std::string &filter)
{
    if (filter.empty())
        return true;

    // Limit search to only direct children for performance
    for (const auto &child : node.children)
    {
        // Check node name
        if (stringContainsFilter(child.name, filter))
            return true;

        // Check filename for files
        if (child.entry && stringContainsFilter(child.entry->filename, filter))
            return true;
    }

    return false;
}

void renderFileTreeNode(const FileTreeNode &node, PakViewerState &state, int depth, int maxDepth)
{
    // Prevent excessive recursion by limiting tree depth
    if (depth >= maxDepth)
        return;

    // Check if this node or any of its children match the filter
    bool nodeMatches = nodeMatchesFilter(node, state.searchFilter);
    bool childrenMatch = false;

    if (!nodeMatches && !state.searchFilter.empty())
    {
        childrenMatch = anyChildrenMatchFilter(node, state.searchFilter);
        if (!childrenMatch)
            return; // Skip this node if neither it nor its children match
    }

    if (node.children.empty())
    {
        // This is a file
        std::string ext = std::filesystem::path(node.name).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        bool isPCX = ext == ".pcx";
        bool isWAL = ext == ".wal";
        bool isSTBImage = ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga";
        bool isText = ext == ".cfg" || ext == ".txt" || ext == ".script" || ext == ".ent" ||
                      ext == ".def" || ext == ".qc" || ext == ".log" || ext == ".ini" ||
                      ext == ".lst" || ext == ".bsp.info" || ext == ".loc" || ext == ".arena" ||
                      ext == ".md" || ext == ".rtf" || ext == ".html" || ext == ".htm" || ext == ".lang";
        bool isBinary = ext == ".dat";
        bool isViewable = isPCX || isWAL || isSTBImage || isText || isBinary;

        // Set text color based on file type
        if (!isViewable)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }
        else if (nodeMatches && !state.searchFilter.empty())
        {
            // Highlight matches
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        }

        if (ImGui::Selectable(node.name.c_str(), state.selectedEntry != -1 &&
                                                     state.entries[state.selectedEntry].filename == node.entry->filename))
        {
            if (isViewable)
            {
                state.selectedEntry = std::find_if(state.entries.begin(), state.entries.end(),
                                                   [&](const PakFileEntry &e)
                                                   { return e.filename == node.entry->filename; }) -
                                      state.entries.begin();
                state.gridView = false; // Switch to single view when selecting an image

                if (isPCX)
                {
                    state.currentImage = PCXParser::loadPCX(state.pakPath, *node.entry);
                    state.currentText = std::nullopt;
                    state.currentBinary = std::nullopt;
                }
                else if (isWAL)
                {
                    state.currentImage = WALParser::loadWAL(state.pakPath, *node.entry);
                    state.currentText = std::nullopt;
                    state.currentBinary = std::nullopt;
                }
                else if (isSTBImage)
                {
                    state.currentImage = STBImageParser::loadSTBImage(state.pakPath, *node.entry);
                    state.currentText = std::nullopt;
                    state.currentBinary = std::nullopt;
                }
                else if (isText)
                {
                    state.currentImage = std::nullopt;
                    state.currentText = TextFileParser::loadTextFile(state.pakPath, *node.entry);
                    state.currentBinary = std::nullopt;
                }
                else if (isBinary)
                {
                    state.currentImage = std::nullopt;
                    state.currentText = std::nullopt;
                    state.currentBinary = BinaryFileParser::loadBinaryFile(state.pakPath, *node.entry);
                }
            }
        }

        // Pop the style color if we pushed it
        if (!isViewable || (nodeMatches && !state.searchFilter.empty()))
        {
            ImGui::PopStyleColor();
        }
    }
    else
    {
        // This is a directory
        // If there's a search filter and this node contains matches, automatically open it
        ImGuiTreeNodeFlags nodeFlags = 0;
        if (!state.searchFilter.empty() && childrenMatch)
            nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

        // Highlight directory if it matches the search
        if (nodeMatches && !state.searchFilter.empty())
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));

        if (ImGui::TreeNodeEx(node.name.c_str(), nodeFlags))
        {
            // Only process children if we haven't exceeded the maximum depth
            if (depth + 1 < maxDepth)
            {
                for (const auto &child : node.children)
                {
                    renderFileTreeNode(child, state, depth + 1, maxDepth);
                }
            }
            else if (depth + 1 == maxDepth)
            {
                // At max depth, just display a message
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Maximum depth reached)");
            }

            ImGui::TreePop();
        }

        // Pop style if we pushed it
        if (nodeMatches && !state.searchFilter.empty())
            ImGui::PopStyleColor();

        // Handle folder selection
        if (ImGui::IsItemClicked())
        {
            state.gridView = true;
            state.currentFolder = node.name;

            // First get filtered files based on search criteria
            auto filteredFiles = getFilteredFiles(node, state.searchFilter);

            // Clear previous images and load new ones
            state.loadedImages.clear();
            loadFilteredImages(filteredFiles, state.pakPath, state.loadedImages);
        }
    }
}

std::string openFileDialog()
{
    const char *file = tinyfd_openFileDialog(
        "Select PAK/PK3 etc. File",
        "",
        0,
        nullptr,
        "All Files",
        0);

    if (file)
    {
        std::string path(file);
        std::cout << "Selected file: " << path << std::endl;
        std::cout << "Extension: " << std::filesystem::path(path).extension().string() << std::endl;
        return path;
    }
    return "";
}

auto renderUI(PakViewerState &state) -> void
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("PAK Adventure", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoNavFocus);

    // Create a horizontal layout for the top bar
    ImGui::BeginChild("TopBar", ImVec2(0, ImGui::GetFrameHeightWithSpacing() + 8), true);

    // Left side: Open PAK File button
    if (ImGui::Button("Open PAK File"))
    {
        std::string selectedFile = openFileDialog();
        if (!selectedFile.empty())
        {
            if (std::filesystem::exists(selectedFile))
            {
                std::string ext = std::filesystem::path(selectedFile).extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                PakFormat format = ParserRegistry::getFormatFromExtension(ext);

                if (format != PakFormat::UNKNOWN)
                {
                    auto loadFunc = ParserRegistry::handlers[format].loadArchive;
                    auto newEntries = loadFunc(selectedFile);

                    if (newEntries)
                    {
                        state.entries = *newEntries;
                        state.pakPath = selectedFile;
                        state.currentImage = std::nullopt;
                        state.selectedEntry = -1;
                        buildFileTree(state.entries, state.fileTree);
                        state.searchFilter = ""; // Clear search filter when loading a new file
                        state.loadedImages.clear();
                        setStatusMessage(state, "File loaded successfully");
                    }
                    else
                    {
                        setStatusMessage(state, "Unknown file type");
                    }
                }
            }
        }
    }

    // Right side: Status message
    ImGui::SameLine();
    float statusWidth = ImGui::GetWindowWidth() - ImGui::GetCursorPosX() - 10.0f;
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - statusWidth - 10.0f);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", state.statusMessage.c_str());

    ImGui::EndChild();

    const float minSidebarWidth = 100.0f;
    const float maxSidebarWidth = 400.0f;

    ImGui::BeginChild("FileList", ImVec2(state.sidebarWidth, 0), true);

    // Add search box at the top of the sidebar
    ImGui::Text("Search:");
    ImGui::SameLine();
    // Create a full-width search box minus some margin
    float searchWidth = ImGui::GetContentRegionAvail().x - 10.0f;
    ImGui::PushItemWidth(searchWidth);

    // Store the previous search filter to detect changes
    std::string prevSearchFilter = state.searchFilter;

    // Limit input length to prevent performance issues with long search terms
    if (state.searchFilter.length() > 50)
    {
        state.searchFilter = state.searchFilter.substr(0, 50);
    }

    // Use a buffer here to avoid issues with the input text callback
    bool searchChanged = ImGui::InputText("##SearchFilter", &state.searchFilter, ImGuiInputTextFlags_AutoSelectAll);

    ImGui::PopItemWidth();

    // Show a search in progress indicator
    static bool searchInProgress = false;
    if (searchChanged)
    {
        searchInProgress = true;

        // Defer grid view update to avoid UI freezing
        if (state.gridView && !state.fileTree.children.empty())
        {
            // Find the current folder node - simplified approach
            FileTreeNode *folderNode = &state.fileTree;

            // Only attempt to find the specific folder if it's a top-level folder
            // This avoids expensive recursive searches
            if (!state.currentFolder.empty())
            {
                for (auto &child : state.fileTree.children)
                {
                    if (child.name == state.currentFolder)
                    {
                        folderNode = &child;
                        break;
                    }
                }
            }

            // Run the improved filtering and loading approach
            auto filteredFiles = getFilteredFiles(*folderNode, state.searchFilter);
            state.loadedImages.clear();
            loadFilteredImages(filteredFiles, state.pakPath, state.loadedImages);
        }
        searchInProgress = false;
    }

    if (searchInProgress)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Searching...");
    }
    else if (!state.searchFilter.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Found %d results", (int)state.loadedImages.size());
    }

    ImGui::Separator();

    if (ImGui::TreeNode("PAK Contents"))
    {
        // Maximum depth for tree rendering to prevent stack overflow
        const int MAX_DEPTH = 10;

        for (const auto &node : state.fileTree.children)
        {
            renderFileTreeNode(node, state, 0, MAX_DEPTH);
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

    // Create a parent container for controls and image view
    ImGui::BeginChild("ContentArea", ImVec2(0, 0), false);

    // Fixed control panel section
    if (state.gridView)
    {
        ImGui::BeginChild("ControlPanel", ImVec2(0, ImGui::GetFrameHeightWithSpacing() + 8), true);
        // Grid view controls
        ImGui::SliderFloat("Grid Scale", &state.gridScale, 0.1f, 2.0f);

        // Show image count in grid view
        if (!state.loadedImages.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "Showing %d image(s)", (int)state.loadedImages.size());
        }

        ImGui::EndChild();
    }

    // Scrollable content area
    ImGui::BeginChild("ImageView", ImVec2(0, 0), true);

    if (state.gridView)
    {
        float windowWidth = ImGui::GetWindowWidth();
        float cellSize = 200.0f * state.gridScale;
        int imagesPerRow = std::max(1, static_cast<int>(windowWidth / cellSize));

        // Defensive check for empty image list
        if (state.loadedImages.empty())
        {
            ImGui::TextWrapped("No images to display in this folder.");
        }
        else
        {
            // Simple grid layout without tables (more efficient)
            int imageCount = state.loadedImages.size();
            ImGui::Columns(imagesPerRow, nullptr, false);

            for (int i = 0; i < imageCount; i++)
            {
                const auto &image = state.loadedImages[i];

                // Calculate image dimensions with aspect ratio
                float imageAspect = (float)image.width / image.height;
                if (imageAspect <= 0.0f)
                    imageAspect = 1.0f;

                float maxImgHeight = cellSize * 0.7f;
                float maxImgWidth = cellSize * 0.9f;

                float imgWidth, imgHeight;
                if (imageAspect > 1.0f)
                {
                    imgWidth = maxImgWidth;
                    imgHeight = imgWidth / imageAspect;
                    if (imgHeight > maxImgHeight)
                    {
                        imgHeight = maxImgHeight;
                        imgWidth = imgHeight * imageAspect;
                    }
                }
                else
                {
                    imgHeight = maxImgHeight;
                    imgWidth = imgHeight * imageAspect;
                    if (imgWidth > maxImgWidth)
                    {
                        imgWidth = maxImgWidth;
                        imgHeight = imgWidth / imageAspect;
                    }
                }

                // Center the image in column
                float colWidth = ImGui::GetColumnWidth();
                float centerOffset = (colWidth - imgWidth) * 0.5f;
                if (centerOffset > 0)
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerOffset);
                }

                // Always render all images
                ImGui::Image((ImTextureID)(uintptr_t)image.textureID, ImVec2(imgWidth, imgHeight));

                // Get filename for label
                std::string filename;
                try
                {
                    filename = image.filename;
                    size_t lastSlash = filename.find_last_of('/');
                    if (lastSlash != std::string::npos && lastSlash < filename.length() - 1)
                        filename = filename.substr(lastSlash + 1);
                }
                catch (...)
                {
                    filename = "[Error]";
                }

                // Simple truncation
                if (filename.length() > 18)
                {
                    filename = filename.substr(0, 15) + "...";
                }

                // Center filename
                float textWidth = ImGui::CalcTextSize(filename.c_str()).x;
                centerOffset = (colWidth - textWidth) * 0.5f;
                if (centerOffset > 0)
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centerOffset);
                }

                ImGui::TextUnformatted(filename.c_str());

                ImGui::NextColumn();
            }

            ImGui::Columns(1);
        }
    }
    else if (state.currentImage)
    {
        // Single image view
        ImGui::Image((ImTextureID)(uintptr_t)state.currentImage->textureID,
                     ImVec2(state.currentImage->width, state.currentImage->height));
    }
    else if (state.currentText)
    {
        // Text file view
        ImGui::BeginChild("TextView", ImVec2(0, 0), true);
        ImGui::TextWrapped("%s", state.currentText->contents.c_str());
        ImGui::EndChild();
    }
    else if (state.currentBinary)
    {
        // Binary file view (hex viewer)
        ImGui::BeginChild("HexView", ImVec2(0, 0), true);

        // File size info
        ImGui::Text("File Size: %zu bytes", state.currentBinary->data.size());

        // Hex viewer settings
        static int bytesPerRow = 16;
        static bool showAscii = true;
        ImGui::SliderInt("Bytes per Row", &bytesPerRow, 4, 32, "%d", ImGuiSliderFlags_None);
        ImGui::SameLine();
        ImGui::Checkbox("Show ASCII", &showAscii);

        // Hex viewer content
        const auto &data = state.currentBinary->data;
        char line[256];
        for (size_t i = 0; i < data.size(); i += bytesPerRow)
        {
            // Address
            snprintf(line, sizeof(line), "%08zX: ", i);
            ImGui::Text("%s", line);

            // Hex values
            for (int j = 0; j < bytesPerRow && (i + j) < data.size(); j++)
            {
                ImGui::SameLine();
                snprintf(line, sizeof(line), "%02X", data[i + j]);
                ImGui::Text("%s", line);
            }

            // ASCII representation
            if (showAscii)
            {
                ImGui::SameLine();
                ImGui::Text("  |");
                for (int j = 0; j < bytesPerRow && (i + j) < data.size(); j++)
                {
                    ImGui::SameLine();
                    char c = data[i + j];
                    if (c >= 32 && c <= 126)
                    {
                        snprintf(line, sizeof(line), "%c", c);
                    }
                    else
                    {
                        snprintf(line, sizeof(line), ".");
                    }
                    ImGui::Text("%s", line);
                }
            }
        }

        ImGui::EndChild();
    }

    ImGui::EndChild();

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

    GLFWwindow *window = glfwCreateWindow(1280, 720, "PAK Adventure", nullptr, nullptr);
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
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
