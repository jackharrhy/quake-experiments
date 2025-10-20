// pak_reader.h
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <filesystem>

namespace PakFinder
{
    struct PakFileEntry
    {
        std::string filename;
        int64_t offset;
        int64_t size;
    };

    struct PakData
    {
        std::filesystem::path pakPath;
        std::vector<PakFileEntry> entries;
    };

    std::optional<PakData> loadPakFile(const std::filesystem::path &filepath);

    std::vector<PakFileEntry> getFileList(const PakData &pak);

    std::optional<std::vector<uint8_t>> loadFile(
        const PakData &pak,
        const std::string &filename);

    extern "C"
    {
        struct PakHandle
        {
            PakData *data;
        };

        PakHandle *PakFinder_loadPakFile(const char *filepath);
        void PakFinder_freePakHandle(PakHandle *handle);
        PakFileEntry *PakFinder_getFileList(PakHandle *handle, int *count);
        uint8_t *PakFinder_loadFile(PakHandle *handle, const char *filename, int64_t *size);
        void PakFinder_freeFileList(PakFileEntry *entries);
        void PakFinder_freeFileData(uint8_t *data);
    }
}