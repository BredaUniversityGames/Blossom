#pragma once
#include <array>
#include <platform/opengl/open_gl.hpp>
#include <math/geometry.hpp>


namespace bee {


enum VertexAttributeIndex
{
    POSITION,
    NORMAL,
    TEXTURE0_UV,
    TEXTURE1_UV,
    TANGENT,
    DISPLACEMENT_UV,

    //Unused for indexing
    MAX_VAL
};

using BufferArray = std::array<GLuint, VertexAttributeIndex::MAX_VAL>;

class Mesh
{
public:
    Mesh(
        GLuint index_buffer_handle,
        uint32_t index_count,
        uint32_t vertex_count,
        GLenum index_format,
        std::array<BufferArray, 3> attribute_buffers,
        uint32_t lodCount,
        GLuint vao_handle,
        BoundingBox bounds
    )

        : index_format(index_format),
        index_count(index_count),
        vertex_count(vertex_count),
        index_handle(index_buffer_handle),
        attribute_buffers(attribute_buffers),
        lodCount(lodCount),
        vao_handle(vao_handle),
        bounds(bounds) 
    {}


    ~Mesh() {
        glDeleteBuffers(1, &index_handle);
        for(uint32_t i = 0; i < lodCount; ++i)
            glDeleteBuffers(VertexAttributeIndex::MAX_VAL, attribute_buffers[i].data());
    }

    GLenum index_format{};
    uint32_t index_count{};
    uint32_t vertex_count{};

    GLuint index_handle{};
    GLuint vao_handle{};
    std::array<BufferArray, 3> attribute_buffers{};
    BoundingBox bounds;

    uint32_t lodCount{ 1 };
};

}