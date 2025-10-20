#include "pak_finder.h"
#include <fstream>
#include <cstring>

#ifdef _MSC_VER
#define PACKED __declspec(align(1))
#else
#define PACKED __attribute__((packed))
#endif

namespace PakFinder
{
    struct PakHeader
    {
        char ident[4]; // "PACK"
        int32_t dirOffset;
        int32_t dirLength;
    } PACKED;

    struct PakEntry
    {
        char filename[56];
        int32_t offset;
        int32_t size;
    } PACKED;

    std::optional<PakData> loadPakFile(const std::filesystem::path &filepath)
    {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            return std::nullopt;
        }

        PakHeader header;
        file.read(reinterpret_cast<char *>(&header), sizeof(PakHeader));

        if (strncmp(header.ident, "PACK", 4) != 0)
        {
            return std::nullopt;
        }

        const int numEntries = header.dirLength / sizeof(PakEntry);
        file.seekg(header.dirOffset);

        std::vector<PakFileEntry> entries;
        entries.reserve(numEntries);

        for (int i = 0; i < numEntries; i++)
        {
            PakEntry entry;
            file.read(reinterpret_cast<char *>(&entry), sizeof(PakEntry));

            entries.push_back({std::string(entry.filename),
                               entry.offset,
                               entry.size});
        }

        return PakData{filepath, std::move(entries)};
    }

    std::vector<PakFileEntry> getFileList(const PakData &pak)
    {
        return pak.entries;
    }

    std::optional<std::vector<uint8_t>> loadFile(
        const PakData &pak,
        const std::string &filename)
    {
        auto it = std::find_if(pak.entries.begin(), pak.entries.end(),
                               [&filename](const PakFileEntry &entry)
                               {
                                   return entry.filename == filename;
                               });

        if (it == pak.entries.end())
        {
            return std::nullopt;
        }

        std::ifstream file(pak.pakPath, std::ios::binary);
        if (!file.is_open())
        {
            return std::nullopt;
        }

        std::vector<uint8_t> buffer(it->size);
        file.seekg(it->offset);
        file.read(reinterpret_cast<char *>(buffer.data()), it->size);

        return buffer;
    }

    extern "C"
    {
        PakHandle *PakFinder_loadPakFile(const char *filepath)
        {
            auto pak = loadPakFile(filepath);
            if (!pak)
                return nullptr;

            PakHandle *handle = new PakHandle{new PakData(std::move(*pak))};
            return handle;
        }

        void PakFinder_freePakHandle(PakHandle *handle)
        {
            if (handle)
            {
                delete handle->data;
                delete handle;
            }
        }

        PakFileEntry *PakFinder_getFileList(PakHandle *handle, int *count)
        {
            if (!handle || !handle->data)
            {
                *count = 0;
                return nullptr;
            }

            const auto &entries = handle->data->entries;
            *count = static_cast<int>(entries.size());
            PakFileEntry *result = new PakFileEntry[entries.size()];

            std::copy(entries.begin(), entries.end(), result);
            return result;
        }

        uint8_t *PakFinder_loadFile(PakHandle *handle, const char *filename, int64_t *size)
        {
            if (!handle || !handle->data)
                return nullptr;

            auto data = loadFile(*handle->data, filename);
            if (!data)
                return nullptr;

            *size = data->size();
            uint8_t *buffer = new uint8_t[*size];
            std::copy(data->begin(), data->end(), buffer);
            return buffer;
        }

        void PakFinder_freeFileList(PakFileEntry *entries)
        {
            delete[] entries;
        }

        void PakFinder_freeFileData(uint8_t *data)
        {
            delete[] data;
        }
    }
}
