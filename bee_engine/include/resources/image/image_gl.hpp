#pragma once
#include <platform/opengl/open_gl.hpp>

namespace bee {

class Image
{
public:
    Image(GLuint glImageHandle, GLenum format, uint32_t width, uint32_t height)
        : handle(glImageHandle), format(format), width(width), height(height) {}

    ~Image() { glDeleteTextures(1, &handle); }

    GLuint handle = -1;
    GLenum format = -1;

    uint32_t width = 0, height = 0;
};

class Image3D : public Image
{
public:
    Image3D(GLuint glImageHandle, GLenum format, uint32_t width, uint32_t height, uint32_t depth) :
        Image(glImageHandle, format, width, height),
        depth(depth)
    {
        
    }

    ~Image3D() = default;

    uint32_t depth;
};

class ImageCubemap : public Image
{
public:
    ImageCubemap(GLuint glImageHandle, GLenum format, uint32_t width, uint32_t height) :
        Image(glImageHandle, format, width, height)
    {

    }

    ~ImageCubemap() = default;
};

}
