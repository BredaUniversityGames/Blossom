#pragma once
#include <string_view>
#include <core/fileio.hpp>
#include <resources/resource_handle.hpp>

namespace bee {

enum class ImageFormat {
    //Four colour channels (byte)
    RGBA8,
    //Single channel image (byte)
    R8,
    //Four colour channels (float)
    RGBA32F,
    //Single channel image (float)
    R32F,
    //Three colour channels (byte) in sRGB colour space
    SRGB8,
    //Four colour channels (float) in sRGB colour space
    SRGBA8
};

class ImageLoader
{
public:

    ImageLoader() = default;
    ~ImageLoader() = default;

    ImageLoader(const ImageLoader&) = delete;
    ImageLoader(ImageLoader&&) = delete;

    ResourceHandle<Image> FromFile(bee::FileIO::Directory directory, std::string_view path, ImageFormat format);

    ResourceHandle<Image> FromRawData(
        const void* image_data, ImageFormat format,
        uint32_t width, uint32_t height
    );

    ResourceHandle<Image3D> FromRawData(
        const void* image_data, ImageFormat format,
        uint32_t width, uint32_t height, uint32_t depth
    );
};

}