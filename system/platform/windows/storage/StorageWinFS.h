#pragma once

#include "StorageBase.h"

#include <cstdio>
#include <filesystem>

namespace PRUZEA {

class StorageWinFS;

class StorageWinFSFile : public StorageBaseFile
{
private:
    std::FILE* handle = nullptr;
    uint32_t fileSize = 0;

public:
    StorageWinFSFile() = default;
    ~StorageWinFSFile() override;

    bool openRead(const std::filesystem::path& path);
    bool openWrite(const std::filesystem::path& path);

    bool isOpen() const override { return mode != OpenMode::CLOSED && handle != nullptr; }
    uint32_t size() const override { return isOpen() ? fileSize : 0; }
    uint32_t read(void* buffer, uint32_t bytes) override;
    uint32_t write(const void* buffer, uint32_t bytes) override;
    void close() override;
    bool closeWrite() override;
};

class StorageWinFS : public StorageBase
{
private:
    std::filesystem::path resourceRoot;
    std::filesystem::path dataRoot;
    StorageWinFSFile fileSlot;
    bool available = false;

    static bool isValidUserFileName(const char* fileName);
    bool makeResourcePath(const char* path, std::filesystem::path& result) const;
    std::filesystem::path gameDataDirectory(const char* gameId) const;

    StorageBaseFile* openWrite(const char* gameId, const char* fileName) override;
    bool onDeleteGameData(const char* gameId) override;

public:
    StorageWinFS() = default;
    ~StorageWinFS() override;

    const char* getName() const override { return "WinFS"; }

    bool begin() override;
    void end() override;
    bool isAvailable() const override { return available; }

    File* openRead(const char* path) override;
    File* openRead(const char* gameId, const char* fileName) override;
    bool directoryExists(const char* path) override;
    bool fileExists(const char* path) override;

    const std::filesystem::path& getDataRoot() const { return dataRoot; }
    const std::filesystem::path& getResourceRoot() const { return resourceRoot; }
};

} // namespace PRUZEA
