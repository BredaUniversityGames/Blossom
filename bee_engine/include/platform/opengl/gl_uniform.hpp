#pragma once
#include <memory>
#include <platform/opengl/open_gl.hpp>

namespace bee
{

template<typename T>
class Uniform
{
public:
    Uniform()
    {
        data = std::make_unique<T>();
        glGenBuffers(1, &buffer);
        Patch();
    }

    ~Uniform()
    {
        glDeleteBuffers(1, &buffer);
    }

    void Patch()
    {
        glBindBuffer(GL_UNIFORM_BUFFER, buffer);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), data.get(), GL_DYNAMIC_DRAW);
        BEE_DEBUG_ONLY(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    }

    void SetName(const std::string& name)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, buffer);
        LabelGL(GL_BUFFER, buffer, name);
        BEE_DEBUG_ONLY(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    }

    Uniform(const Uniform<T>& other) = delete;

    T* operator->()
    {
        return data.get();
    }

    T* get() { return data.get(); }

    GLuint buffer;

private:
    std::unique_ptr<T> data;
};

}
