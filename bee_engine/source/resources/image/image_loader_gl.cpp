#include <precompiled/engine_precompiled.hpp>
#include <resources/image/image_loader.hpp>
#include <resources/image/image_gl.hpp>
#include <resources/image/image_common.hpp>

#include <tinygltf/stb_image.h>

#include <core/engine.hpp>

bee::ResourceHandle<bee::Image> bee::ImageLoader::FromFile(bee::FileIO::Directory directory, std::string_view path, ImageFormat format)
{
    std::string fullpath = bee::Engine.FileIO().GetPath(directory, std::string(path));

    int desired_channels{};
    bool isFloat{};
    switch (format)
    {
    case bee::ImageFormat::RGBA8:
        desired_channels = 4;
        break;
    case bee::ImageFormat::R8:
        desired_channels = 1;
        break;
    case bee::ImageFormat::RGBA32F:
        desired_channels = 4;
        isFloat = true;
        break;
    case bee::ImageFormat::R32F:
        desired_channels = 1;
        isFloat = true;
        break;
    default:
        throw std::runtime_error("Unsupported format requested");
        break;
    }

    void* stbiResult = nullptr;
    int height{}, width{}, component{};

    {
        auto buffer = bee::Engine.FileIO().ReadBinaryFile(fullpath);

        if (isFloat)
            stbiResult = stbi_loadf_from_memory(
                reinterpret_cast<unsigned char*>(buffer.data()),
                static_cast<int>(buffer.size()), &width, &height, &component, desired_channels
            );
        else
            stbiResult = stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(buffer.data()),
                static_cast<int>(buffer.size()), &width, &height, &component, desired_channels
            );

    }

    if (stbiResult == nullptr)
    {
        throw std::runtime_error("Image failed loading - STBI error");
    }

    auto ret = FromRawData(stbiResult, format, width, height);

    ret.SetPath(std::string(path));
    bee::LabelGL(GL_TEXTURE, ret.Retrieve()->handle, std::string(path));

    stbi_image_free(stbiResult);
    return ret;
}

bee::ResourceHandle<bee::Image> bee::ImageLoader::FromRawData(const void* image_data, ImageFormat format, uint32_t width, uint32_t height)
{ 

    GLint gl_format{};
    GLint usage{};
    GLint type{};

    //TODO: Expand format choice
    //TODO: Add format flags
    switch (format)
    {
    case ImageFormat::RGBA8:
        gl_format = GL_RGBA8;
        usage = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    case ImageFormat::R8:
        gl_format = GL_R8;
        usage = GL_RED;
        type = GL_UNSIGNED_BYTE;
        break;
    case ImageFormat::RGBA32F:
        gl_format = GL_RGBA32F;
        usage = GL_RGBA;
        type = GL_FLOAT;
        break;
    case ImageFormat::R32F:
        gl_format = GL_R8;
        usage = GL_RED;
        type = GL_FLOAT;
        break;
    case ImageFormat::SRGB8:
        gl_format = GL_SRGB8;
        usage = GL_RGB;
        type = GL_UNSIGNED_BYTE;
        break;
    case ImageFormat::SRGBA8:
        gl_format = GL_SRGB8_ALPHA8;
        usage = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    default:
        throw std::runtime_error("Encountered unsupported image data");
        break;
    }

    GLuint textureHandle{};
    glGenTextures(1, &textureHandle);             // Gen
    glBindTexture(GL_TEXTURE_2D, textureHandle);  // Bind

    glTexImage2D(
        GL_TEXTURE_2D,     // What (target)
        0,                 // Mip-map level
        gl_format,         // Internal format
        width,             // Width
        height,            // Height
        0,                 // Border
        usage,             // Format (how to use)
        type,              // Type   (how to interpret)
        image_data
    );

    glGenerateMipmap(GL_TEXTURE_2D);
    
    auto new_entry = std::make_shared<ResourceEntry<Image>>();
    new_entry->resource = std::make_shared<Image>(textureHandle, gl_format, width, height);

    return { new_entry };
}


bee::ResourceHandle<bee::Image3D> bee::ImageLoader::FromRawData(const void* image_data, ImageFormat format, uint32_t width, uint32_t height, uint32_t depth)
{

    GLint gl_format{};
    GLint usage{};
    GLint type{};

    //TODO: Expand format choice
    //TODO: Add format flags
    switch (format)
    {
    case ImageFormat::RGBA8:
        gl_format = GL_RGBA8;
        usage = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    case ImageFormat::R8:
        gl_format = GL_R8;
        usage = GL_RED;
        type = GL_UNSIGNED_BYTE;
        break;
    case ImageFormat::RGBA32F:
        gl_format = GL_RGBA8;
        usage = GL_RGBA;
        type = GL_FLOAT;
        break;
    case ImageFormat::R32F:
        gl_format = GL_R8;
        usage = GL_RED;
        type = GL_FLOAT;
        break;
    default:
        throw std::runtime_error("Encountered unsupported image data");
        break;
    }

    GLuint textureHandle{};
    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_3D, textureHandle);

    glTexImage3D(
        GL_TEXTURE_3D,     // What (target)
        0,                 // Mip-map level
        gl_format,         // Internal format
        width,             // Width
        height,            // Height
        depth,             // Depth
        0,                 // Border
        usage,             // Format (how to use)
        type,              // Type   (how to interpret)
        image_data
    );

    glGenerateMipmap(GL_TEXTURE_3D);

    auto new_entry = std::make_shared<ResourceEntry<Image3D>>();
    new_entry->resource = std::make_shared<Image3D>(textureHandle, gl_format, width, height, depth);

    return { new_entry };
}

float bee::image_utils::GetAspectRatio(bee::ResourceHandle<bee::Image> image)
{
    auto texture = image.Retrieve();
    return static_cast<float>(texture->width) / static_cast<float>(texture->height);
}
