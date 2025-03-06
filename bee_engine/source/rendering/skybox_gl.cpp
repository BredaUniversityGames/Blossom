#include <precompiled/engine_precompiled.hpp>
#include <platform/opengl/open_gl.hpp>

#include "platform/opengl/shader_gl.hpp"
#include "rendering/skybox.hpp"
#include "resources/image/image_gl.hpp"

constexpr glm::vec3 skyboxVertices[] = {
    // -Z   
    glm::vec3{ -1.0f,  1.0f, -1.0f },
    glm::vec3{ -1.0f, -1.0f, -1.0f },
    glm::vec3{  1.0f, -1.0f, -1.0f },
    glm::vec3{  1.0f, -1.0f, -1.0f },
    glm::vec3{  1.0f,  1.0f, -1.0f },
    glm::vec3{ -1.0f,  1.0f, -1.0f },

    // -X
    glm::vec3{ -1.0f, -1.0f,  1.0f },
    glm::vec3{ -1.0f, -1.0f, -1.0f },
    glm::vec3{ -1.0f,  1.0f, -1.0f },
    glm::vec3{ -1.0f,  1.0f, -1.0f },
    glm::vec3{ -1.0f,  1.0f,  1.0f },
    glm::vec3{ -1.0f, -1.0f,  1.0f },
    
    // +X
    glm::vec3{  1.0f, -1.0f, -1.0f },
    glm::vec3{  1.0f, -1.0f,  1.0f },
    glm::vec3{  1.0f,  1.0f,  1.0f },
    glm::vec3{  1.0f,  1.0f,  1.0f },
    glm::vec3{  1.0f,  1.0f, -1.0f },
    glm::vec3{  1.0f, -1.0f, -1.0f },

    // +Z
    glm::vec3{ -1.0f, -1.0f,  1.0f },
    glm::vec3{ -1.0f,  1.0f,  1.0f },
    glm::vec3{  1.0f,  1.0f,  1.0f },
    glm::vec3{  1.0f,  1.0f,  1.0f },
    glm::vec3{  1.0f, -1.0f,  1.0f },
    glm::vec3{ -1.0f, -1.0f,  1.0f },

    // +Y
    glm::vec3{ -1.0f,  1.0f, -1.0f },
    glm::vec3{  1.0f,  1.0f, -1.0f },
    glm::vec3{  1.0f,  1.0f,  1.0f },
    glm::vec3{  1.0f,  1.0f,  1.0f },
    glm::vec3{ -1.0f,  1.0f,  1.0f },
    glm::vec3{ -1.0f,  1.0f, -1.0f },

    // -Y
    glm::vec3{ -1.0f, -1.0f, -1.0f },
    glm::vec3{ -1.0f, -1.0f,  1.0f },
    glm::vec3{  1.0f, -1.0f, -1.0f },
    glm::vec3{  1.0f, -1.0f, -1.0f },
    glm::vec3{ -1.0f, -1.0f,  1.0f },
    glm::vec3{  1.0f, -1.0f,  1.0f },
};

class bee::Skybox::Impl
{
public:
    GLuint m_vbo;
    GLuint m_vao;
    std::shared_ptr<ImageCubemap> m_cubemapSkybox;

    void CreateCubemap();
};

bee::Skybox::Skybox(ResourceHandle<Image> skyboxImage, std::shared_ptr<Shader> skyboxPass) :
    m_impl(std::make_unique<Skybox::Impl>()),
    m_skyboxPass(skyboxPass)
{
    // Generate box buffers.
    glGenVertexArrays(1, &m_impl->m_vao);
    glGenBuffers(1, &m_impl->m_vbo);
    glBindVertexArray(m_impl->m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_impl->m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));

    // Set sampler texture unit on shader.
    m_skyboxPass->Activate();
    m_skyboxPass->GetParameter("u_skybox")->SetValue(0);
    m_skyboxPass->Deactivate();

    m_impl->m_cubemapSkybox = std::make_shared<ImageCubemap>(-1, GL_RGBA32F, m_size, m_size);

    SetSkybox(skyboxImage);
}

bee::Skybox::~Skybox()
{
    // Clean up buffers and textures.
    glDeleteVertexArrays(1, &m_impl->m_vao);
    glDeleteBuffers(1, &m_impl->m_vbo);
    glDeleteTextures(1, &m_impl->m_cubemapSkybox->handle);
}

void bee::Skybox::SetSkybox(ResourceHandle<Image> skyboxImage)
{
    // Create cubemap textures.
    m_impl->CreateCubemap();

    // Create framebuffer for the HDRI to render to
    GLuint frameBuffer;

    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_impl->m_cubemapSkybox->handle, 0);

    // Render into cubemap using HDRI.
    ConvertHDRI(skyboxImage);

    // Clean up framebuffer.
    glDeleteFramebuffers(1, &frameBuffer);
}

void bee::Skybox::Render() const
{
    PushDebugGL("Skybox pass");

    // Set depth function to only render behind all scene geometry.
    glDepthFunc(GL_LEQUAL);

    m_skyboxPass->Activate();

    glBindVertexArray(m_impl->m_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_impl->m_cubemapSkybox->handle);

    // Draw skybox.
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(std::size(skyboxVertices)));

    m_skyboxPass->Deactivate();
    glBindVertexArray(0);

    // Reset depth function.
    glDepthFunc(GL_LESS);

    PopDebugGL();
}

void bee::Skybox::Impl::CreateCubemap()
{
    glGenTextures(1, &m_cubemapSkybox->handle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapSkybox->handle);

    // MIP maps had issues in the past with clearly showing the corners.
    // When turning this on, investigate that bug further.
    bool withMipMaps = false;

    // Create cubemap faces.
    for(int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_cubemapSkybox->format, m_cubemapSkybox->width, m_cubemapSkybox->height, 0, GL_RGBA, GL_FLOAT, nullptr);
    }

    if(withMipMaps)
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // Set sampler parameters.
    GLint minFilter = withMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    GLint magFilter = GL_LINEAR;

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void bee::Skybox::ConvertHDRI(ResourceHandle<Image> skyboxImage)
{
    // Load shader for conversion.
    std::shared_ptr<Shader> shader{std::make_shared<Shader>(
        FileIO::Directory::Asset, 
        "shaders/skybox/fullscreen.vert", 
        "shaders/skybox/hdr_to_cubemap.frag"
    )};

    for(int i = 0; i < 6; ++i)
    {
        // Bind cubemap face to framebuffer.
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, 
            GL_COLOR_ATTACHMENT0, 
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            m_impl->m_cubemapSkybox->handle, 
            0);

        glBindTexture(GL_TEXTURE_CUBE_MAP, m_impl->m_cubemapSkybox->handle);

        // Clear framebuffer.
        glViewport(0, 0, m_size, m_size);
        glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update shader uniforms and textures.
        shader->Activate();

        glActiveTexture(GL_TEXTURE0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, skyboxImage.Retrieve()->handle);

        shader->GetParameter("u_face")->SetValue(i);
        shader->GetParameter("u_hdr")->SetValue(0);

        // Perform draw into framebuffer.
        // Uses bufferless rendering: https://trass3r.github.io/coding/2019/09/11/bufferless-rendering.html
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    glGenerateTextureMipmap(m_impl->m_cubemapSkybox->handle);
    shader->Deactivate();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::shared_ptr<bee::ImageCubemap> bee::Skybox::GetSkyboxCubemap()
{
    return m_impl->m_cubemapSkybox;
}