#include "PRUZEA.h"
#include "PRUZEAConfig.h"
#if PRUZEA_ENABLE_PSRAM
#include "hardware/psram.h"
#include "pico/platform/sections.h"
#endif
#include <LovyanGFX.hpp>
#include <lgfx/utility/lgfx_pngle.h>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <new>

using namespace PRUZEA;

namespace {

#if PRUZEA_ENABLE_PSRAM
struct alignas(max_align_t) PsramBlock
{
    size_t size;
    PsramBlock* next;
    bool free;
};

alignas(max_align_t) uint8_t psramImageArena[PICO_PSRAM_SIZE_BYTES] __uninitialized_psram("pruzea_image");
PsramBlock* firstPsramBlock = nullptr;
bool psramInitializationAttempted = false;

constexpr size_t alignPsramSize(size_t size)
{
    constexpr size_t ALIGNMENT = alignof(max_align_t);
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

bool initializePsramImageAllocator()
{
    if (psramInitializationAttempted) return firstPsramBlock != nullptr;
    psramInitializationAttempted = true;

    if (!psram_is_available() || !psram_check_address(psramImageArena) ||
        !psram_check_address(psramImageArena + sizeof(psramImageArena) - 1)) return false;

    firstPsramBlock = reinterpret_cast<PsramBlock*>(psramImageArena);
    firstPsramBlock->size = sizeof(psramImageArena) - sizeof(PsramBlock);
    firstPsramBlock->next = nullptr;
    firstPsramBlock->free = true;
    return true;
}

void mergeFreePsramImageBlocks()
{
    for (PsramBlock* block = firstPsramBlock; block != nullptr && block->next != nullptr;)
    {
        if (block->free && block->next->free)
        {
            block->size += sizeof(PsramBlock) + block->next->size;
            block->next = block->next->next;
        }
        else
        {
            block = block->next;
        }
    }
}

void* allocatePsramImageBuffer(size_t size)
{
    if (size == 0 || !initializePsramImageAllocator()) return nullptr;
    size = alignPsramSize(size);

    for (PsramBlock* block = firstPsramBlock; block != nullptr; block = block->next)
    {
        if (!block->free || block->size < size) continue;

        if (block->size >= size + sizeof(PsramBlock) + alignof(max_align_t))
        {
            auto* next = reinterpret_cast<PsramBlock*>(reinterpret_cast<uint8_t*>(block + 1) + size);
            next->size = block->size - size - sizeof(PsramBlock);
            next->next = block->next;
            next->free = true;
            block->size = size;
            block->next = next;
        }

        block->free = false;
        return block + 1;
    }

    return nullptr;
}

void freePsramImageBuffer(void* buffer)
{
    if (buffer == nullptr || firstPsramBlock == nullptr || !psram_check_address(buffer)) return;

    auto* block = static_cast<PsramBlock*>(buffer) - 1;
    const uintptr_t blockAddress = reinterpret_cast<uintptr_t>(block);
    const uintptr_t arenaStart = reinterpret_cast<uintptr_t>(psramImageArena);
    const uintptr_t arenaEnd = arenaStart + sizeof(psramImageArena);
    if (blockAddress < arenaStart || blockAddress + sizeof(PsramBlock) > arenaEnd) return;

    block->free = true;
    mergeFreePsramImageBlocks();
}
#endif

struct ImageLayout
{
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    int32_t offsetX = 0;
    int32_t offsetY = 0;
};

ImageLayout calculateImageLayout(uint32_t sourceWidth, uint32_t sourceHeight, uint16_t outputWidth, uint16_t outputHeight, Graphics::Image::Fit fit)
{
    ImageLayout layout;
    layout.scaleX = static_cast<float>(outputWidth) / sourceWidth;
    layout.scaleY = static_cast<float>(outputHeight) / sourceHeight;
    if (fit == Graphics::Image::Fit::STRETCH) return layout;

    const float scale = fit == Graphics::Image::Fit::CONTAIN
        ? std::min(layout.scaleX, layout.scaleY)
        : std::max(layout.scaleX, layout.scaleY);
    layout.scaleX = scale;
    layout.scaleY = scale;

    const int32_t scaledWidth = static_cast<int32_t>(std::ceil(sourceWidth * scale));
    const int32_t scaledHeight = static_cast<int32_t>(std::ceil(sourceHeight * scale));
    layout.offsetX = (static_cast<int32_t>(outputWidth) - scaledWidth) / 2;
    layout.offsetY = (static_cast<int32_t>(outputHeight) - scaledHeight) / 2;
    return layout;
}

bool isJpegStartOfFrame(uint8_t marker)
{
    return
        (marker >= 0xC0 && marker <= 0xC3) ||
        (marker >= 0xC5 && marker <= 0xC7) ||
        (marker >= 0xC9 && marker <= 0xCB) ||
        (marker >= 0xCD && marker <= 0xCF);
}

uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return
        static_cast<uint16_t>((red & 0xF8) << 8) |
        static_cast<uint16_t>((green & 0xFC) << 3) |
        static_cast<uint16_t>(blue >> 3);
}

bool readJpegSize(const uint8_t* data, uint32_t size, uint16_t& outputWidth, uint16_t& outputHeight)
{
    if (data == nullptr || size < 4 || data[0] != 0xFF || data[1] != 0xD8) return false;

    uint32_t offset = 2;
    while (offset + 1 < size)
    {
        while (offset < size && data[offset] != 0xFF) ++offset;
        while (offset < size && data[offset] == 0xFF) ++offset;
        if (offset >= size) return false;

        const uint8_t marker = data[offset++];
        if (marker == 0xD8 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) continue;
        if (marker == 0xD9 || marker == 0xDA || offset + 1 >= size) return false;

        const uint16_t segmentLength = static_cast<uint16_t>(data[offset] << 8) | data[offset + 1];
        if (segmentLength < 2 || segmentLength > size - offset) return false;

        if (isJpegStartOfFrame(marker))
        {
            if (segmentLength < 7) return false;
            outputHeight = static_cast<uint16_t>(data[offset + 3] << 8) | data[offset + 4];
            outputWidth = static_cast<uint16_t>(data[offset + 5] << 8) | data[offset + 6];
            return outputWidth > 0 && outputHeight > 0;
        }
        offset += segmentLength;
    }
    return false;
}

struct PngDecodeState
{
    lgfx::LGFX_Sprite* sprite = nullptr;
    const uint8_t* data = nullptr;
    uint32_t size = 0;
    uint32_t offset = 0;
    ImageLayout layout;
};

uint32_t readPngData(void* userData, uint8_t* buffer, uint32_t length)
{
    auto* state = static_cast<PngDecodeState*>(userData);
    const uint32_t available = state->offset < state->size ? state->size - state->offset : 0;
    const uint32_t readLength = std::min(length, available);
    if (buffer != nullptr && readLength > 0) std::memcpy(buffer, state->data + state->offset, readLength);
    state->offset += readLength;
    return readLength;
}

void drawPngData(void* userData, uint32_t x, uint32_t y, uint_fast8_t divX, size_t length, const uint8_t* argb)
{
    auto* state = static_cast<PngDecodeState*>(userData);
    if (state == nullptr || state->sprite == nullptr || argb == nullptr) return;

    const int32_t destinationY0 = state->layout.offsetY + static_cast<int32_t>(std::ceil(y * state->layout.scaleY));
    const int32_t destinationY1 = state->layout.offsetY + static_cast<int32_t>(std::ceil((y + 1) * state->layout.scaleY));
    if (destinationY0 >= destinationY1) return;

    for (size_t i = 0; i < length; ++i)
    {
        const int32_t destinationX0 = state->layout.offsetX + static_cast<int32_t>(std::ceil(x * state->layout.scaleX));
        const int32_t destinationX1 = state->layout.offsetX + static_cast<int32_t>(std::ceil((x + divX) * state->layout.scaleX));
        if (destinationX0 < destinationX1)
        {
            state->sprite->fillRect(
                destinationX0,
                destinationY0,
                destinationX1 - destinationX0,
                destinationY1 - destinationY0,
                rgb565(argb[1], argb[2], argb[3]));
        }
        x += divX;
        argb += 4;
    }
}

bool decodeJpeg(lgfx::LGFX_Sprite& sprite, uint16_t width, uint16_t height, const uint8_t* data, uint32_t size, Graphics::Image::Fit fit)
{
    uint16_t sourceWidth = 0;
    uint16_t sourceHeight = 0;
    if (!readJpegSize(data, size, sourceWidth, sourceHeight)) return false;

    const ImageLayout layout = calculateImageLayout(sourceWidth, sourceHeight, width, height, fit);
    return sprite.drawJpg(
        data,
        size,
        std::max<int32_t>(layout.offsetX, 0),
        std::max<int32_t>(layout.offsetY, 0),
        width,
        height,
        std::max<int32_t>(-layout.offsetX, 0),
        std::max<int32_t>(-layout.offsetY, 0),
        layout.scaleX,
        layout.scaleY,
        lgfx::datum_t::top_left);
}

bool decodePng(lgfx::LGFX_Sprite& sprite, uint16_t width, uint16_t height, const uint8_t* data, uint32_t size, Graphics::Image::Fit fit)
{
    PngDecodeState state;
    state.sprite = &sprite;
    state.data = data;
    state.size = size;

    pngle_t* decoder = lgfx_pngle_new();
    if (decoder == nullptr) return false;
    if (lgfx_pngle_prepare(decoder, readPngData, &state) < 0)
    {
        lgfx_pngle_destroy(decoder);
        return false;
    }

    const uint32_t sourceWidth = lgfx_pngle_get_width(decoder);
    const uint32_t sourceHeight = lgfx_pngle_get_height(decoder);
    if (sourceWidth == 0 || sourceHeight == 0)
    {
        lgfx_pngle_destroy(decoder);
        return false;
    }

    state.layout = calculateImageLayout(sourceWidth, sourceHeight, width, height, fit);
    const bool decoded = lgfx_pngle_decomp(decoder, drawPngData) >= 0;
    lgfx_pngle_destroy(decoder);
    return decoded;
}

class ImageLGFX : public Graphics::Image
{
public:
    bool load(const uint8_t* data, uint32_t size, uint16_t outputWidth, uint16_t outputHeight, Fit fit, bool png)
    {
        if (data == nullptr || size == 0 || outputWidth == 0 || outputHeight == 0) return false;

        release();

        sprite = new (std::nothrow) lgfx::LGFX_Sprite(nullptr);
        if (sprite == nullptr) return false;

        sprite->setColorDepth(lgfx::color_depth_t::rgb565_nonswapped);
#if PRUZEA_ENABLE_PSRAM
        const size_t pixelCount = static_cast<size_t>(outputWidth) * outputHeight;
        imageBuffer = allocatePsramImageBuffer(pixelCount * sizeof(uint16_t));
        if (imageBuffer != nullptr) sprite->setBuffer(imageBuffer, outputWidth, outputHeight);
#endif
        if (imageBuffer == nullptr && sprite->createSprite(outputWidth, outputHeight) == nullptr)
        {
            release();
            return false;
        }
        sprite->fillScreen(Graphics::BLACK);

        const bool decoded = png
            ? decodePng(*sprite, outputWidth, outputHeight, data, size, fit)
            : decodeJpeg(*sprite, outputWidth, outputHeight, data, size, fit);
        if (!decoded)
        {
            release();
            return false;
        }

        width = outputWidth;
        height = outputHeight;
        return true;
    }

    const uint16_t* getBitmap() const override { return static_cast<const uint16_t*>(sprite->getBuffer()); }
    uint16_t getWidth() const override { return width; };
    uint16_t getHeight() const override { return height; };

    void close() override
    {
        release();
        Graphics::Image::close();
    }

private:
    void release()
    {
        if (sprite != nullptr)
        {
            sprite->deleteSprite();
            delete sprite;
            sprite = nullptr;
        }
#if PRUZEA_ENABLE_PSRAM
        freePsramImageBuffer(imageBuffer);
#endif
        imageBuffer = nullptr;
    }

    uint16_t width = 0;
    uint16_t height = 0;
    lgfx::LGFX_Sprite* sprite = nullptr;
    void* imageBuffer = nullptr;
};

} // namespace

Graphics::Image* Graphics::Image::loadJpeg(const uint8_t* jpegData, uint32_t jpegSize, uint16_t outputWidth, uint16_t outputHeight, Fit fit)
{
    auto *image = new (std::nothrow) ImageLGFX();
    if (image == nullptr) return nullptr;

    if (image->load(jpegData, jpegSize, outputWidth, outputHeight, fit, false))
    {
        return image;
    }
    else
    {
        delete image;
        return nullptr;
    }
}

Graphics::Image* Graphics::Image::loadPng(const uint8_t* pngData, uint32_t pngSize, uint16_t outputWidth, uint16_t outputHeight, Fit fit)
{
    auto *image = new (std::nothrow) ImageLGFX();
    if (image == nullptr) return nullptr;

    if (image->load(pngData, pngSize, outputWidth, outputHeight, fit, true))
    {
        return image;
    }
    else
    {
        delete image;
        return nullptr;
    }
}

void Graphics::Image::close() 
{
    delete this;
}
