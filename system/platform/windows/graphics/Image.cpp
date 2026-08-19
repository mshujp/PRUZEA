#include "PRUZEA.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>
#include <wincodec.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <new>
#include <vector>

using namespace PRUZEA;

namespace
{

class ComScope
{
public:
    ComScope()
    {
        const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        initialized = result == S_OK || result == S_FALSE;
        usable = initialized || result == RPC_E_CHANGED_MODE;
    }

    ~ComScope()
    {
        if (initialized) CoUninitialize();
    }

    bool isUsable() const { return usable; }

private:
    bool initialized = false;
    bool usable = false;
};

template<typename T>
void releaseCom(T*& object)
{
    if (object == nullptr) return;
    object->Release();
    object = nullptr;
}

uint16_t toRgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return static_cast<uint16_t>((red & 0xF8u) << 8) |
        static_cast<uint16_t>((green & 0xFCu) << 3) |
        static_cast<uint16_t>(blue >> 3);
}

bool isMagentaFringe(uint8_t red, uint8_t green, uint8_t blue)
{
    const int minimumRedBlue = std::min<int>(red, blue);
    const int redBlueDifference = std::abs(static_cast<int>(red) - static_cast<int>(blue));
    return minimumRedBlue >= 48 && green * 3 < minimumRedBlue && redBlueDifference <= std::max(24, minimumRedBlue / 2);
}

class ImageWindows : public Graphics::Image
{
public:
    ImageWindows() = default;
    ~ImageWindows() override = default;

    bool load(const uint8_t* data, uint32_t size, uint16_t outputWidth, uint16_t outputHeight, Fit fit, bool png)
    {
        if (data == nullptr || size == 0 || outputWidth == 0 || outputHeight == 0) return false;

        ComScope com;
        if (!com.isUsable()) return false;

        IWICImagingFactory* factory = nullptr;
        IWICStream* stream = nullptr;
        IWICBitmapDecoder* decoder = nullptr;
        IWICBitmapFrameDecode* frame = nullptr;
        IWICFormatConverter* converter = nullptr;

        HRESULT result = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        if (SUCCEEDED(result)) result = factory->CreateStream(&stream);
        if (SUCCEEDED(result)) result = stream->InitializeFromMemory(const_cast<BYTE*>(data), size);
        if (SUCCEEDED(result)) result = factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder);
        if (SUCCEEDED(result)) result = decoder->GetFrame(0, &frame);
        if (SUCCEEDED(result)) result = factory->CreateFormatConverter(&converter);
        if (SUCCEEDED(result))
        {
            result = converter->Initialize(
                frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
        }

        UINT sourceWidth = 0;
        UINT sourceHeight = 0;
        if (SUCCEEDED(result)) result = converter->GetSize(&sourceWidth, &sourceHeight);

        const uint64_t sourceBytes64 = static_cast<uint64_t>(sourceWidth) * sourceHeight * 4u;
        if (sourceWidth == 0 || sourceHeight == 0 || sourceBytes64 > std::numeric_limits<UINT>::max()) result = E_FAIL;

        std::vector<uint8_t> sourcePixels;
        if (SUCCEEDED(result))
        {
            try
            {
                sourcePixels.resize(static_cast<size_t>(sourceBytes64));
            }
            catch (...)
            {
                result = E_OUTOFMEMORY;
            }
        }
        if (SUCCEEDED(result))
        {
            result = converter->CopyPixels(
                nullptr, sourceWidth * 4u, static_cast<UINT>(sourceBytes64), sourcePixels.data());
        }

        releaseCom(converter);
        releaseCom(frame);
        releaseCom(decoder);
        releaseCom(stream);
        releaseCom(factory);
        if (FAILED(result)) return false;

        try
        {
            const uint16_t backgroundColor = static_cast<uint16_t>(png ? Graphics::MAGENTA : Graphics::BLACK);
            bitmap.assign(static_cast<size_t>(outputWidth) * outputHeight, backgroundColor);
        }
        catch (...)
        {
            return false;
        }

        float scaleX = static_cast<float>(outputWidth) / sourceWidth;
        float scaleY = static_cast<float>(outputHeight) / sourceHeight;
        if (fit != Fit::STRETCH)
        {
            const float scale = fit == Fit::CONTAIN ? std::min(scaleX, scaleY) : std::max(scaleX, scaleY);
            scaleX = scale;
            scaleY = scale;
        }

        const float scaledWidth = sourceWidth * scaleX;
        const float scaledHeight = sourceHeight * scaleY;
        const float offsetX = (outputWidth - scaledWidth) * 0.5f;
        const float offsetY = (outputHeight - scaledHeight) * 0.5f;

        for (uint16_t y = 0; y < outputHeight; ++y)
        {
            const int32_t sourceY = static_cast<int32_t>((static_cast<float>(y) + 0.5f - offsetY) / scaleY);
            if (sourceY < 0 || sourceY >= static_cast<int32_t>(sourceHeight)) continue;

            for (uint16_t x = 0; x < outputWidth; ++x)
            {
                const int32_t sourceX = static_cast<int32_t>((static_cast<float>(x) + 0.5f - offsetX) / scaleX);
                if (sourceX < 0 || sourceX >= static_cast<int32_t>(sourceWidth)) continue;

                const size_t sourceIndex = (static_cast<size_t>(sourceY) * sourceWidth + sourceX) * 4u;
                const uint8_t red = sourcePixels[sourceIndex];
                const uint8_t green = sourcePixels[sourceIndex + 1];
                const uint8_t blue = sourcePixels[sourceIndex + 2];
                const uint8_t alpha = sourcePixels[sourceIndex + 3];
                bitmap[static_cast<size_t>(y) * outputWidth + x] = png && (alpha < 255 || isMagentaFringe(red, green, blue))
                    ? static_cast<uint16_t>(Graphics::MAGENTA)
                    : toRgb565(red, green, blue);
            }
        }

        width = outputWidth;
        height = outputHeight;
        return true;
    }

    const uint16_t* getBitmap() const override { return bitmap.data(); }
    uint16_t getWidth() const override { return width; }
    uint16_t getHeight() const override { return height; }

    void close() override
    {
        bitmap.clear();
        Graphics::Image::close();
    }

private:
    std::vector<uint16_t> bitmap;
    uint16_t width = 0;
    uint16_t height = 0;
};

Graphics::Image* loadImage(
    const uint8_t* data, uint32_t size, uint16_t outputWidth, uint16_t outputHeight, Graphics::Image::Fit fit, bool png)
{
    auto* image = new (std::nothrow) ImageWindows();
    if (image == nullptr) return nullptr;
    if (image->load(data, size, outputWidth, outputHeight, fit, png)) return image;

    delete image;
    return nullptr;
}

} // namespace

Graphics::Image* Graphics::Image::loadJpeg(
    const uint8_t* jpegData, uint32_t jpegSize, uint16_t outputWidth, uint16_t outputHeight, Fit fit)
{
    return loadImage(jpegData, jpegSize, outputWidth, outputHeight, fit, false);
}

Graphics::Image* Graphics::Image::loadPng(
    const uint8_t* pngData, uint32_t pngSize, uint16_t outputWidth, uint16_t outputHeight, Fit fit)
{
    return loadImage(pngData, pngSize, outputWidth, outputHeight, fit, true);
}

void Graphics::Image::close()
{
    delete this;
}
