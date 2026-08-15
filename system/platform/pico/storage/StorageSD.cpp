#include "StorageSD.h"
#include "hw_config.h"

#include <cstring>
#include <cstdio>
#include <pico/time.h>

namespace PRUZEA {

namespace {
    spi_t       g_spi    = {};
    sd_spi_if_t g_spiIf  = {};
    sd_card_t   g_sdCard = {};
    FATFS       g_fatFs;
}

} // namespace PRUZEA

extern "C" size_t sd_get_num()
{
    return 1;
}

extern "C" sd_card_t* sd_get_by_num(size_t num)
{
    return (num == 0) ? &PRUZEA::g_sdCard : nullptr;
}

namespace PRUZEA {

StorageSDFile::~StorageSDFile()
{
    close();
}

bool StorageSDFile::openRead(const char* path)
{
    if (mode != OpenMode::CLOSED) close();
    if (path == nullptr) return false;

    FRESULT fr = f_open(&file, path, FA_READ);
    if (fr != FR_OK)
    {
        mode = OpenMode::CLOSED;
        fileSize = 0;
        return false;
    }

    mode = OpenMode::READ;
    fileSize = static_cast<uint32_t>(f_size(&file));
    return true;
}

bool StorageSDFile::openWrite(const char* path)
{
    if (mode != OpenMode::CLOSED) close();
    if (path == nullptr) return false;

    const FRESULT fr = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        mode = OpenMode::CLOSED;
        fileSize = 0;
        return false;
    }

    mode = OpenMode::WRITE;
    fileSize = 0;
    return true;
}

uint32_t StorageSDFile::read(void* buffer, uint32_t bytes)
{
    if (mode != OpenMode::READ || buffer == nullptr)
    {
        return 0;
    }

    UINT bytesRead = 0;
    FRESULT fr = f_read(&file, buffer, static_cast<UINT>(bytes), &bytesRead);
    if (fr != FR_OK)
    {
        return 0;
    }

    return static_cast<uint32_t>(bytesRead);
}

uint32_t StorageSDFile::write(const void* buffer, uint32_t bytes)
{
    if (mode != OpenMode::WRITE || (buffer == nullptr && bytes > 0)) return 0;

    UINT bytesWritten = 0;
    const FRESULT fr = f_write(&file, buffer, static_cast<UINT>(bytes), &bytesWritten);
    if (fr != FR_OK) return 0;

    fileSize += static_cast<uint32_t>(bytesWritten);
    return static_cast<uint32_t>(bytesWritten);
}

void StorageSDFile::close()
{
    const bool wasOpen = mode != OpenMode::CLOSED;
    if (wasOpen)
    {
        f_close(&file);
        mode = OpenMode::CLOSED;
        fileSize = 0;
    }
    if (wasOpen && owner != nullptr) owner->unmountCard();
}

bool StorageSDFile::closeWrite()
{
    if (mode != OpenMode::WRITE) return false;

    const FRESULT syncResult = f_sync(&file);
    const FRESULT closeResult = f_close(&file);
    mode = OpenMode::CLOSED;
    fileSize = 0;
    if (owner != nullptr) owner->unmountCard();
    return syncResult == FR_OK && closeResult == FR_OK;
}


StorageSD::StorageSD(const Config& config)
    : spiHost(config.spiHost), pinMiso(config.misoPin), pinSck(config.sckPin),
      pinMosi(config.mosiPin), pinCs(config.csPin), baudRate(config.baudRate),
      fileSlot(this)
{
}

bool StorageSD::makeFatPath(const char* path, char* outBuffer, size_t bufferSize) const
{
    if (path == nullptr || outBuffer == nullptr || bufferSize == 0)
    {
        return false;
    }

    outBuffer[0] = '\0';

    if (path[0] == '\0')
    {
        return false;
    }

    // Already a FatFs volume path, e.g. "0:/PRUZEA_Games".
    if (path[0] >= '0' && path[0] <= '9' && path[1] == ':')
    {
        const int written = snprintf(outBuffer, bufferSize, "%s", path);
        return written >= 0 && written < static_cast<int>(bufferSize);
    }

    // PRUZEA public paths are written as "/PRUZEA_Games/...".
    // FatFs paths are internally converted to "0:/PRUZEA_Games/...".
    const int written = snprintf(outBuffer, bufferSize, "%s%s", kVolumePath, path);
    return written >= 0 && written < static_cast<int>(bufferSize);
}

bool StorageSD::begin()
{
    if (driverReady) return isAvailable();
    if (spiHost > 1 || pinMiso < 0 || pinSck < 0 || pinMosi < 0 || pinCs < 0) return false;

    g_spi = {};
    g_spi.hw_inst   = (spiHost == 1) ? spi1 : spi0;
    g_spi.miso_gpio = static_cast<uint>(pinMiso);
    g_spi.mosi_gpio = static_cast<uint>(pinMosi);
    g_spi.sck_gpio  = static_cast<uint>(pinSck);
    g_spi.baud_rate = baudRate;

    g_spiIf = {};
    g_spiIf.spi     = &g_spi;
    g_spiIf.ss_gpio = static_cast<uint>(pinCs);

    g_sdCard = {};
    g_sdCard.type            = SD_IF_SPI;
    g_sdCard.spi_if_p        = &g_spiIf;
    g_sdCard.use_card_detect = false;

    if (!sd_init_driver())
    {
        driverReady = false;
        return false;
    }

    driverReady = true;
    availabilityCached = false;

    if (!mountCard())
    {
        driverReady = false;
        return false;
    }

    const bool rootReady = ensureDirectory(ROOT_DIR);
    unmountCard();
    updateAvailabilityCache(rootReady);
    if (!rootReady)
    {
        end();
        return false;
    }

    return true;
}

void StorageSD::end()
{
    fileSlot.close();
    unmountCard();
    driverReady = false;
    availabilityCached = false;
    cachedAvailable = false;
    lastAvailabilityCheckMsec = 0;
}

bool StorageSD::mountCard()
{
    if (!driverReady) return false;
    if (mounted) return true;

    const FRESULT fr = f_mount(&g_fatFs, kVolumePath, 1);
    if (fr != FR_OK)
    {
        f_unmount(kVolumePath);
        updateAvailabilityCache(false);
        return false;
    }

    mounted = true;
    updateAvailabilityCache(true);
    return true;
}

void StorageSD::unmountCard()
{
    if (!mounted) return;
    f_unmount(kVolumePath);
    mounted = false;
}

void StorageSD::updateAvailabilityCache(bool available) const
{
    cachedAvailable = available;
    availabilityCached = true;
    lastAvailabilityCheckMsec = static_cast<uint32_t>(to_ms_since_boot(get_absolute_time()));
}

bool StorageSD::isAvailable() const
{
    if (!driverReady) return false;

    const uint32_t now = static_cast<uint32_t>(to_ms_since_boot(get_absolute_time()));
    if (availabilityCached &&
        static_cast<uint32_t>(now - lastAvailabilityCheckMsec) < AVAILABILITY_CACHE_MSEC)
    {
        return cachedAvailable;
    }

    StorageSD* self = const_cast<StorageSD*>(this);
    const bool wasMounted = self->mounted;
    const bool available = self->mountCard();
    if (!wasMounted && available) self->unmountCard();
    self->updateAvailabilityCache(available);
    return available;
}

bool StorageSD::ensureDirectory(const char* path)
{
    if (!mounted || path == nullptr || path[0] == '\0')
    {
        return false;
    }

    char fatPath[FAT_PATH_MAX];
    if (!makeFatPath(path, fatPath, sizeof(fatPath)))
    {
        return false;
    }

    size_t len = std::strlen(fatPath);
    if (len == 0 || len >= sizeof(fatPath))
    {
        return false;
    }

    // Skip "0:".
    size_t start = 0;
    if (len >= 2 && fatPath[1] == ':')
    {
        start = 2;
    }

    // Create each intermediate directory.
    // Example: 0:/PRUZEA_Games/foo/bar
    for (size_t i = start + 1; i < len; ++i)
    {
        if (fatPath[i] == '/')
        {
            fatPath[i] = '\0';

            FRESULT fr = f_mkdir(fatPath);
            if (fr != FR_OK && fr != FR_EXIST)
            {
                return false;
            }

            fatPath[i] = '/';
        }
    }

    FRESULT fr = f_mkdir(fatPath);
    if (fr != FR_OK && fr != FR_EXIST)
    {
        return false;
    }

    return true;
}

Storage::File* StorageSD::openRead(const char* path)
{
    if (path == nullptr) return nullptr;

    fileSlot.close();
    if (!mountCard()) return nullptr;

    char fatPath[FAT_PATH_MAX];
    if (!makeFatPath(path, fatPath, sizeof(fatPath)))
    {
        unmountCard();
        return nullptr;
    }

    if (!fileSlot.openRead(fatPath))
    {
        unmountCard();
        return nullptr;
    }

    return &fileSlot;
}

bool StorageSD::isValidUserFileName(const char* fileName)
{
    if (fileName == nullptr || fileName[0] == '\0')
    {
        return false;
    }

    size_t length = 0;

    for (const char* p = fileName; *p != '\0'; ++p)
    {
        const unsigned char c =
            static_cast<unsigned char>(*p);

        if (++length >= FAT_PATH_MAX ||
            c == '/'  ||
            c == '\\' ||
            c == ':'  ||
            c < 0x20  ||
            c == 0x7f)
        {
            return false;
        }
    }

    return true;
}

Storage::File* StorageSD::openRead(const char* gameId, const char* fileName)
{
    if (!isValidUserFileName(fileName)) return nullptr;

    fileSlot.close();
    if (!mountCard()) return nullptr;

    char dataDir[FAT_PATH_MAX];
    if (!getDataDir(dataDir, sizeof(dataDir), gameId))
    {
        unmountCard();
        return nullptr;
    }

    char path[FAT_PATH_MAX];
    const int written = snprintf(path, sizeof(path), "%s/%s", dataDir, fileName);
    if (written < 0 || written >= static_cast<int>(sizeof(path)))
    {
        unmountCard();
        return nullptr;
    }

    char fatPath[FAT_PATH_MAX];
    if (!makeFatPath(path, fatPath, sizeof(fatPath)) || !fileSlot.openRead(fatPath))
    {
        unmountCard();
        return nullptr;
    }
    return &fileSlot;
}

bool StorageSD::getDataDir(char* outBuffer, uint16_t bufferSize, const char* gameId)
{
    if (outBuffer == nullptr || bufferSize == 0) return false;
    outBuffer[0] = '\0';

    if (!mounted || !isValidGameId(gameId)) return false;

    const int written = snprintf(outBuffer, bufferSize, "%s/%s", ROOT_DIR, gameId);
    if (written < 0 || written >= static_cast<int>(bufferSize))
    {
        outBuffer[0] = '\0';
        return false;
    }

    return ensureDirectory(outBuffer);
}

bool StorageSD::deleteDirectoryRecursive(const char* fatPath)
{
    DIR directory = {};
    if (f_opendir(&directory, fatPath) != FR_OK) return false;

    bool result = true;
    while (result)
    {
        FILINFO info = {};
        if (f_readdir(&directory, &info) != FR_OK)
        {
            result = false;
            break;
        }

        if (info.fname[0] == '\0') break;
        if (std::strcmp(info.fname, ".") == 0 || std::strcmp(info.fname, "..") == 0) continue;

        char childPath[FAT_PATH_MAX];
        const int written = snprintf(childPath, sizeof(childPath), "%s/%s", fatPath, info.fname);
        if (written < 0 || written >= static_cast<int>(sizeof(childPath)))
        {
            result = false;
            break;
        }

        result = (info.fattrib & AM_DIR) != 0
            ? deleteDirectoryRecursive(childPath)
            : f_unlink(childPath) == FR_OK;
    }

    if (f_closedir(&directory) != FR_OK) result = false;
    if (!result) return false;
    return f_unlink(fatPath) == FR_OK;
}

bool StorageSD::onDeleteGameData(const char* gameId)
{
    fileSlot.close();
    if (!mountCard()) return false;

    char path[FAT_PATH_MAX];
    const int written = snprintf(path, sizeof(path), "%s/%s", ROOT_DIR, gameId);

    char fatPath[FAT_PATH_MAX];
    bool result = written >= 0 &&
        written < static_cast<int>(sizeof(path)) &&
        makeFatPath(path, fatPath, sizeof(fatPath));

    if (result)
    {
        FILINFO info = {};
        const FRESULT statResult = f_stat(fatPath, &info);

        if (statResult == FR_NO_FILE || statResult == FR_NO_PATH)
        {
            result = true;
        }
        else if (statResult != FR_OK || (info.fattrib & AM_DIR) == 0)
        {
            result = false;
        }
        else
        {
            result = deleteDirectoryRecursive(fatPath);
        }
    }

    unmountCard();
    return result;
}

StorageBaseFile* StorageSD::openWrite(const char* gameId, const char* fileName)
{
    if (!isValidUserFileName(fileName)) return nullptr;

    fileSlot.close();
    if (!mountCard()) return nullptr;

    char dataDir[FAT_PATH_MAX];
    if (!getDataDir(dataDir, sizeof(dataDir), gameId))
    {
        unmountCard();
        return nullptr;
    }

    char path[FAT_PATH_MAX];
    const int pathLength = snprintf(path, sizeof(path), "%s/%s", dataDir, fileName);
    if (pathLength < 0 || pathLength >= static_cast<int>(sizeof(path)))
    {
        unmountCard();
        return nullptr;
    }

    char fatPath[FAT_PATH_MAX];
    if (!makeFatPath(path, fatPath, sizeof(fatPath)))
    {
        unmountCard();
        return nullptr;
    }

    if (!fileSlot.openWrite(fatPath))
    {
        unmountCard();
        return nullptr;
    }

    return &fileSlot;
}

bool StorageSD::directoryExists(const char* path)
{
    if (path == nullptr) return false;

    fileSlot.close();
    if (!mountCard()) return false;

    char fatPath[FAT_PATH_MAX];
    if (!makeFatPath(path, fatPath, sizeof(fatPath)))
    {
        unmountCard();
        return false;
    }

    FILINFO fno;
    FRESULT fr = f_stat(fatPath, &fno);
    const bool exists = fr == FR_OK && (fno.fattrib & AM_DIR) != 0;
    unmountCard();
    return exists;
}

bool StorageSD::fileExists(const char* path)
{
    if (path == nullptr) return false;

    fileSlot.close();
    if (!mountCard()) return false;

    char fatPath[FAT_PATH_MAX];
    if (!makeFatPath(path, fatPath, sizeof(fatPath)))
    {
        unmountCard();
        return false;
    }

    FILINFO fno;
    FRESULT fr = f_stat(fatPath, &fno);
    const bool exists = fr == FR_OK && (fno.fattrib & AM_DIR) == 0;
    unmountCard();
    return exists;
}

} // namespace PRUZEA
