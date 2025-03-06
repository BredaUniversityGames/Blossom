#include "precompiled/engine_precompiled.hpp"

#include "core/engine.hpp"
#include "platform/opengl/open_gl.hpp"
#include "platform/opengl/shader_gl.hpp"
#include "rendering/ibl_renderer.hpp"
#include "rendering/shader_db.hpp"
#include "platform/opengl/uniforms_gl.hpp"
#include "resources/image/image_gl.hpp"
#include <tools/log.hpp>

class bee::IBLRenderer::Impl
{
public:
    void CreateDiffuseIBL(std::shared_ptr<Image> diffuseIBL, std::shared_ptr<Image> envCubemap, uint32_t textureSize);
    void CreateSpecularIBL(std::shared_ptr<Image> specularIBL, std::shared_ptr<Image> envCubemap, uint32_t textureSize, uint32_t specularMipCount);
    void CreateLUTIBL(std::shared_ptr<Image> lutIBL, std::shared_ptr<Image> envCubemap, uint32_t textureSize);

    unsigned int m_captureFBO;
    unsigned int m_captureRBO;

    const int sampleCount = 1024;
};

bee::IBLRenderer::IBLRenderer() : m_impl(std::make_unique<Impl>())
{
    m_specularMipCount = (int)floor(std::log2(m_textureSizeSpecular)) + 1 - m_lowestMipLevel;

    glGenFramebuffers(1, &m_impl->m_captureFBO);
    glGenRenderbuffers(1, &m_impl->m_captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_impl->m_captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_impl->m_captureRBO);
}

void bee::IBLRenderer::Render(std::shared_ptr<Image> envCubemap, const Material::IBL& ibl)
{
    auto t = std::chrono::high_resolution_clock::now();

    m_impl->CreateDiffuseIBL(ibl.diffuse, envCubemap, m_textureSizeDiffuse);
    m_impl->CreateSpecularIBL(ibl.specular, envCubemap, m_textureSizeSpecular, m_specularMipCount);
    m_impl->CreateLUTIBL(ibl.LUT, envCubemap, m_textureSizeLut);

    glFinish();
    Log::Info("IBL generation: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t).count());

}

void bee::IBLRenderer::Impl::CreateDiffuseIBL(std::shared_ptr<Image> diffuseIBL, std::shared_ptr<Image> envCubemap, uint32_t textureSize)
{
    std::shared_ptr<Shader> filterIBL = Engine.ShaderDB()[ShaderDB::Type::FILTER_IBL];

    glBindTexture(GL_TEXTURE_CUBE_MAP, diffuseIBL->handle);
    for (int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, textureSize, textureSize, 0, GL_RGB, GL_FLOAT,
                     nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_CUBE_MAP, diffuseIBL->handle);

    filterIBL->Activate();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap->handle);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    filterIBL->GetParameter("u_generate_lut")->SetValue(0);
    filterIBL->GetParameter("u_lod_bias")->SetValue(2.0f);
    filterIBL->GetParameter("u_sample_count")->SetValue(sampleCount);
    filterIBL->GetParameter("u_distribution")->SetValue(0);  // c_Lambert = 0
    filterIBL->GetParameter("u_roughness")->SetValue(0.0f);
    filterIBL->GetParameter("u_width")->SetValue(static_cast<int>(textureSize));
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    glViewport(0, 0, textureSize, textureSize);
    for (int i = 0; i < 6; ++i)
    {
        filterIBL->GetParameter("u_current_face")->SetValue(i);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, diffuseIBL->handle, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        RenderQuad();
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap->handle);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void bee::IBLRenderer::Impl::CreateSpecularIBL(std::shared_ptr<Image> specularIBL, std::shared_ptr<Image> envCubemap, uint32_t textureSize, uint32_t specularMipCount)
{
    std::shared_ptr<Shader> filterIBL = Engine.ShaderDB()[ShaderDB::Type::FILTER_IBL];

    glBindTexture(GL_TEXTURE_CUBE_MAP, specularIBL->handle);
    for (int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, textureSize, textureSize, 0, GL_RGB, GL_FLOAT,
                     nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_CUBE_MAP, specularIBL->handle);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    filterIBL->Activate();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap->handle);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    filterIBL->GetParameter("u_generate_lut")->SetValue(0);
    filterIBL->GetParameter("u_lod_bias")->SetValue(0.0f);
    filterIBL->GetParameter("u_sample_count")->SetValue(sampleCount);
    filterIBL->GetParameter("u_distribution")->SetValue(1);  // c_GGX = 1

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    for (uint32_t level = 0; level < specularMipCount; level++)
    {
        float roughness = static_cast<float>(level) / (static_cast<float>(specularMipCount - 1));
        uint32_t width = textureSize >> level;

        filterIBL->GetParameter("u_roughness")->SetValue(roughness);
        filterIBL->GetParameter("u_width")->SetValue(static_cast<int>(textureSize));
        glViewport(0, 0, width, width);
        for (int i = 0; i < 6; ++i)
        {
            filterIBL->GetParameter("u_current_face")->SetValue(i);
            glFramebufferTexture2D(GL_FRAMEBUFFER, 
                                   GL_COLOR_ATTACHMENT0, 
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                                   specularIBL->handle,
                                   level);
            glClear(GL_COLOR_BUFFER_BIT);
            RenderQuad();
        }
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap->handle);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void bee::IBLRenderer::Impl::CreateLUTIBL(std::shared_ptr<Image> lutIBL, std::shared_ptr<Image> envCubemap, uint32_t textureSize)
{
    std::shared_ptr<Shader> filterIBL = Engine.ShaderDB()[ShaderDB::Type::FILTER_IBL];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lutIBL->handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureSize, textureSize, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    LabelGL(GL_TEXTURE, lutIBL->handle, "[R] IBL LUT");

    filterIBL->Activate();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap->handle);
    glUniform1i(0, 0);
    filterIBL->GetParameter("u_generate_lut")->SetValue(1);  // true
    filterIBL->GetParameter("u_lod_bias")->SetValue(0.0f);
    filterIBL->GetParameter("u_sample_count")->SetValue(sampleCount);
    filterIBL->GetParameter("u_distribution")
        ->SetValue(1);  // c_GGX = 1			filterIBL->GetParameter("u_roughness")->SetValue(roughness);
    filterIBL->GetParameter("u_width")->SetValue(static_cast<int>(textureSize));
    filterIBL->GetParameter("u_current_face")->SetValue(0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    glViewport(0, 0, textureSize, textureSize);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lutIBL->handle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lutIBL->handle, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    RenderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bee::IBLRenderer::~IBLRenderer() = default;

