#include "StorageWinFS.h"

#include <SDL3/SDL.h>

#include <cstring>
#include <system_error>

using namespace PRUZEA;

StorageWinFSFile::~StorageWinFSFile()
{
    close();
}

bool StorageWinFSFile::openRead(const std::filesystem::path& path)
{
    close();

#ifdef _WIN32
    handle = _wfopen(path.c_str(), L"rb");
#else
    handle = std::fopen(path.string().c_str(), "rb");
#endif
    if (handle == nullptr) return false;

    if (std::fseek(handle, 0, SEEK_END) != 0)
    {
        close();
        return false;
    }

    const long length = std::ftell(handle);
    if (length < 0 || std::fseek(handle, 0, SEEK_SET) != 0)
    {
        close();
        return false;
    }

    fileSize = static_cast<uint32_t>(length);
    mode = OpenMode::READ;
    return true;
}

bool StorageWinFSFile::openWrite(const std::filesystem::path& path)
{
    close();

#ifdef _WIN32
    handle = _wfopen(path.c_str(), L"wb");
#else
    handle = std::fopen(path.string().c_str(), "wb");
#endif
    if (handle == nullptr) return false;

    fileSize = 0;
    mode = OpenMode::WRITE;
    return true;
}

uint32_t StorageWinFSFile::read(void* buffer, uint32_t bytes)
{
    if (mode != OpenMode::READ || handle == nullptr || buffer == nullptr) return 0;
    return static_cast<uint32_t>(std::fread(buffer, 1, bytes, handle));
}

uint32_t StorageWinFSFile::write(const void* buffer, uint32_t bytes)
{
    if (mode != OpenMode::WRITE || handle == nullptr || (buffer == nullptr && bytes > 0)) return 0;

    const size_t written = std::fwrite(buffer, 1, bytes, handle);
    fileSize += static_cast<uint32_t>(written);
    return static_cast<uint32_t>(written);
}

void StorageWinFSFile::close()
{
    if (handle != nullptr)
    {
        std::fclose(handle);
        handle = nullptr;
    }

    mode = OpenMode::CLOSED;
    fileSize = 0;
}

bool StorageWinFSFile::closeWrite()
{
    if (mode != OpenMode::WRITE || handle == nullptr) return false;

    const bool flushOk = std::fflush(handle) == 0;
    const bool closeOk = std::fclose(handle) == 0;
    handle = nullptr;
    mode = OpenMode::CLOSED;
    fileSize = 0;
    return flushOk && closeOk;
}

StorageWinFS::~StorageWinFS()
{
    end();
}

bool StorageWinFS::begin()
{
    if (available) return true;

    const char* basePath = SDL_GetBasePath();
    if (basePath == nullptr) return false;
    resourceRoot = std::filesystem::u8path(basePath);

    char* prefPath = SDL_GetPrefPath("PRUZEA", "PRUZEA");
    if (prefPath == nullptr) return false;

    dataRoot = std::filesystem::u8path(prefPath);
    SDL_free(prefPath);

    std::error_code ec;
    std::filesystem::create_directories(dataRoot, ec);
    available = !ec;
    return available;
}

void StorageWinFS::end()
{
    fileSlot.close();
    available = false;
}

bool StorageWinFS::isValidUserFileName(const char* fileName)
{
    if (fileName == nullptr || fileName[0] == '\0') return false;

    for (const unsigned char* p =
             reinterpret_cast<const unsigned char*>(fileName);
         *p != 0;
         ++p)
    {
        if (*p < 0x20 || *p == 0x7f ||
            *p == '/' || *p == '\\' || *p == ':')
        {
            return false;
        }
    }

    return true;
}

std::filesystem::path StorageWinFS::gameDataDirectory(const char* gameId) const
{
    return dataRoot / std::filesystem::u8path(gameId);
}

bool StorageWinFS::makeResourcePath(const char* path, std::filesystem::path& result) const
{
    if (path == nullptr || path[0] == '\0') return false;

    std::filesystem::path relative = std::filesystem::u8path(path);
    if (relative.has_root_name()) return false;
    if (relative.has_root_directory()) relative = relative.relative_path();
    relative = relative.lexically_normal();

    for (const auto& part : relative)
    {
        if (part == "..") return false;
    }

    result = resourceRoot / relative;
    return true;
}

Storage::File* StorageWinFS::openRead(const char* path)
{
    if (!available) return nullptr;

    fileSlot.close();

    std::filesystem::path resourcePath;
    if (!makeResourcePath(path, resourcePath) || !fileSlot.openRead(resourcePath)) return nullptr;
    return &fileSlot;
}

Storage::File* StorageWinFS::openRead(const char* gameId, const char* fileName)
{
    if (!available || !isValidGameId(gameId) || !isValidUserFileName(fileName))
    {
        return nullptr;
    }

    fileSlot.close();
    if (!fileSlot.openRead(gameDataDirectory(gameId) / std::filesystem::u8path(fileName)))
    {
        return nullptr;
    }

    return &fileSlot;
}

StorageBaseFile* StorageWinFS::openWrite(const char* gameId, const char* fileName)
{
    if (!available || !isValidGameId(gameId) || !isValidUserFileName(fileName))
    {
        return nullptr;
    }

    fileSlot.close();

    const auto dir = gameDataDirectory(gameId);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) return nullptr;

    if (!fileSlot.openWrite(dir / std::filesystem::u8path(fileName)))
    {
        return nullptr;
    }

    return &fileSlot;
}

bool StorageWinFS::onDeleteGameData(const char* gameId)
{
    if (!available || !isValidGameId(gameId)) return false;

    fileSlot.close();

    std::error_code ec;
    const auto dir = gameDataDirectory(gameId);
    if (!std::filesystem::exists(dir, ec)) return !ec;

    std::filesystem::remove_all(dir, ec);
    return !ec;
}

bool StorageWinFS::directoryExists(const char* path)
{
    if (!available) return false;

    std::filesystem::path resourcePath;
    if (!makeResourcePath(path, resourcePath)) return false;
    std::error_code ec;
    return std::filesystem::is_directory(resourcePath, ec) && !ec;
}

bool StorageWinFS::fileExists(const char* path)
{
    if (!available) return false;

    std::filesystem::path resourcePath;
    if (!makeResourcePath(path, resourcePath)) return false;
    std::error_code ec;
    return std::filesystem::is_regular_file(resourcePath, ec) && !ec;
}
