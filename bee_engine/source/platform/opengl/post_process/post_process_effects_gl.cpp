#include <precompiled/engine_precompiled.hpp>
#include "rendering/post_process/post_process_effects.hpp"

#include <code_utils/bee_utils.hpp>
#include "core/device.hpp"
#include "core/engine.hpp"
#include <core/fileio.hpp>
#include <platform/opengl/open_gl.hpp>
#include <platform/opengl/shader_gl.hpp>
#include "rendering/shader_db.hpp"
#include "core/ecs.hpp"
#include "rendering/render_components.hpp"
#include "core/transform.hpp"

class bee::Bloom::Impl
{
public:
    static const int m_max_pingpong = 2;

    unsigned int m_pingpongFramebuffers[m_max_pingpong];
    unsigned int m_pingpongColorbuffers[m_max_pingpong];
};

class bee::DepthOfField::Impl
{
public:
    static const int s_flipCount = 2;

    unsigned int m_blurFramebuffers[s_flipCount];
    unsigned int m_blurTargetTexture[s_flipCount];
};

// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------

using namespace bee;

PostProcess::PostProcess()
{
	m_width = Engine.Device().GetWidth();
	m_height = Engine.Device().GetHeight();
}

// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------

Vignette::Vignette(float vignette)
{
    m_type = PostProcess::Type::Vignette;

	m_vignetteShader = std::make_shared<Shader>(FileIO::Directory::Asset, "/shaders/postprocess/standard.vert", "shaders/postprocess/vignette.frag");
    
    m_vignetteData.m_vignette = vignette;
}

Vignette::~Vignette()
{

}

void Vignette::Draw(const RenderTextureCollection& rtCollection, void* dstFramebuffer)
{
    PushDebugGL("Vignette pass");
    Draw(rtCollection.sourceTexture, dstFramebuffer, rtCollection.brightnessTexture);
    PopDebugGL();
}

void Vignette::Draw(void* sourceTexture, void* finalFramebuffer, void* brightnessTexture)
{
    unsigned int sourceTex = *static_cast<unsigned int*>(sourceTexture);
    unsigned int finalBuf = *static_cast<unsigned int*>(finalFramebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, finalBuf);
    glViewport(0, 0, m_width, m_height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTex);

    m_vignetteShader->Activate();
    m_vignetteShader->GetParameter("u_vignette")->SetValue(m_vignetteData.m_vignette);
    bee::RenderQuad();
}

// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------

Bloom::Bloom(float weight, unsigned int blur)
{
    m_type = PostProcess::Type::Bloom;

    m_impl = std::make_unique<Bloom::Impl>();
    m_screenQuad = std::make_shared<Shader>(FileIO::Directory::Asset, "shaders/postprocess/standard.vert", "shaders/postprocess/screen_quad.frag");
    m_gaussianBlur = std::make_shared<Shader>(FileIO::Directory::Asset, "shaders/postprocess/standard.vert", "shaders/postprocess/gaussian_blur.frag");
    m_bloomFinal = std::make_shared<Shader>(FileIO::Directory::Asset, "shaders/postprocess/standard.vert", "shaders/postprocess/bloom_final.frag");

    m_bloomData.m_blurAmount = blur;
    m_bloomData.m_bloomWeight = weight;

    // -- pingpong buffers --
    glGenFramebuffers(2, &m_impl->m_pingpongFramebuffers[0]);
    glGenTextures(2, &m_impl->m_pingpongColorbuffers[0]);
    for (int i = 0; i < m_impl->m_max_pingpong; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_pingpongFramebuffers[i]);
        LabelGL(GL_FRAMEBUFFER, m_impl->m_pingpongFramebuffers[i], ("[P] Bloom Pingpong Frame buffer" + std::to_string(i)).c_str());
        glBindTexture(GL_TEXTURE_2D, m_impl->m_pingpongColorbuffers[i]);
        LabelGL(GL_TEXTURE, m_impl->m_pingpongColorbuffers[i], ("[P] Bloom Pingpong Color buffer" + std::to_string(i)).c_str());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_impl->m_pingpongColorbuffers[i], 0);

        // Check that our framebuffer is OK
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) assert(false);
        BEE_DEBUG_ONLY(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }
}

Bloom::~Bloom()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteTextures(2, &m_impl->m_pingpongColorbuffers[0]);
    glDeleteFramebuffers(2, &m_impl->m_pingpongFramebuffers[0]);
}

void Bloom::Draw(const RenderTextureCollection& rtCollection, void* dstFramebuffer)
{
    PushDebugGL("Bloom pass");

    unsigned int sourceTex = *static_cast<unsigned int*>(rtCollection.sourceTexture);
    unsigned int brightnessTex = *static_cast<unsigned int*>(rtCollection.brightnessTexture);
    unsigned int finalBuf = *static_cast<unsigned int*>(dstFramebuffer);

    // blur bright fragments with two-pass Gaussian blur
    bool horizontal = true, first_iteration = true;
    unsigned int amount = m_bloomData.m_blurAmount * 2;
    m_gaussianBlur->Activate();
    for (unsigned int i = 0; i < amount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_pingpongFramebuffers[horizontal]);
        m_gaussianBlur->GetParameter("u_horizontal")->SetValue(horizontal);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? brightnessTex : m_impl->m_pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
        bee::RenderQuad();
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // final render to quad
    glBindFramebuffer(GL_FRAMEBUFFER, finalBuf);
    m_bloomFinal->Activate();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTex);
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_impl->m_pingpongColorbuffers[!horizontal]);
    glUniform1i(1, 1);
    m_bloomFinal->GetParameter("u_weight")->SetValue(m_bloomData.m_bloomWeight);
    bee::RenderQuad();

    PopDebugGL();
}

// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------

DepthOfField::DepthOfField()
{
    m_type = PostProcess::Type::DepthOfField;
    m_impl = std::make_unique<Impl>();

    m_dofData.FocusDistance = 25.0f;
    m_dofData.FocusFalloffDistance = 40.0f;
    m_dofData.BlurStrength = 0.1f;

    glGenFramebuffers(m_impl->s_flipCount, m_impl->m_blurFramebuffers);
    glGenTextures(m_impl->s_flipCount, m_impl->m_blurTargetTexture);

    glActiveTexture(GL_TEXTURE0);
    for (int i = 0; i < m_impl->s_flipCount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_blurFramebuffers[i]);
        LabelGL(GL_FRAMEBUFFER, m_impl->m_blurFramebuffers[i], ("[P] DOF Blur framebuffer " + std::to_string(i)).c_str());

        glBindTexture(GL_TEXTURE_2D, m_impl->m_blurTargetTexture[i]);
        LabelGL(GL_TEXTURE, m_impl->m_blurTargetTexture[i], ("[P] DOF Blur target texture " + std::to_string(i)).c_str());

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_impl->m_blurTargetTexture[i], 0);

        // Check that our framebuffer is OK
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) assert(false);
        BEE_DEBUG_ONLY(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

DepthOfField::~DepthOfField()
{

}

void DepthOfField::Draw(const RenderTextureCollection& rtCollection, void* dstFramebuffer)
{
    PushDebugGL("Depth of Field");
    PushDebugGL("Gaussian");

    unsigned int sourceTex = *static_cast<unsigned int*>(rtCollection.sourceTexture);
    unsigned int wsposTex = *static_cast<unsigned int*>(rtCollection.worldSpacePositionTexture);
    unsigned int finalBuf = *static_cast<unsigned int*>(dstFramebuffer);

    // TODO: Reduce texture fetches from 9 to 5 by exploiting bilinear filtering
    // https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/

    std::shared_ptr<Shader> nineTapGauss = Engine.ShaderDB()[ShaderDB::Type::GAUSSIAN_9TAP_FILTER];

    nineTapGauss->Activate();


    const int BLUR_HORIZONTAL = 0;
    const int BLUR_VERTICAL = 1;

    const int tapMultiplier = static_cast<int>(std::max(std::min(m_dofData.BlurStrength,1.0f), 0.01f) * 80.0f);
    int writeIndex = 0, readIndex = 1;

    glActiveTexture(GL_TEXTURE0);
    for (int i = 0; i < 2 * tapMultiplier; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_blurFramebuffers[writeIndex]);

        int readTex = i == 0 ? sourceTex : m_impl->m_blurTargetTexture[readIndex];

        glBindTexture(GL_TEXTURE_2D, readTex);
        glm::vec2 blur_dir = 
            writeIndex == BLUR_HORIZONTAL ? 
            glm::vec2(1.0f, 0.0f) : glm::vec2(0.0f, 1.0f);

        nineTapGauss->GetParameter("u_blur_direction")->SetValue(blur_dir);

        RenderQuad();

        // Flip buffers
        writeIndex = (writeIndex + 1) % 2;
        readIndex = (readIndex + 1) % 2;
    }

    PopDebugGL();

    PushDebugGL("Composite");
    glBindFramebuffer(GL_FRAMEBUFFER, finalBuf);

    std::shared_ptr<Shader> compositeShader = Engine.ShaderDB()[ShaderDB::Type::DOF_COMPOSITE];

    compositeShader->Activate();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_impl->m_blurTargetTexture[readIndex]);
    glUniform1i(0, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sourceTex);
    glUniform1i(1, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, wsposTex);
    glUniform1i(2, 2);

    auto cameraView = Engine.ECS().Registry.view<Transform, CameraComponent>();

    if (cameraView.begin() != cameraView.end())
    {
        auto cameraTransform = Engine.ECS().Registry.get<Transform>(cameraView.front());

        compositeShader->GetParameter("u_camera_pos")->SetValue(cameraTransform.GetTranslation());
    }

    compositeShader->GetParameter("u_focus_distance")->SetValue(m_dofData.FocusDistance);
    compositeShader->GetParameter("u_focus_fallof_distance")->SetValue(m_dofData.FocusFalloffDistance);


    RenderQuad();

    PopDebugGL();
    PopDebugGL();
}