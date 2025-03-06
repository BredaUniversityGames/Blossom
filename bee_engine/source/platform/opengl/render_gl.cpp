#include <precompiled/engine_precompiled.hpp>
#include "rendering/render.hpp"

#include <tinygltf/stb_image.h>  // Implementation of stb_image is in gltf_loader.cpp

#include <glm/glm.hpp>
#include <rendering/debug_render.hpp>
#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "ui/ui.hpp"
#include <resources/resource_manager.hpp>
#include "core/transform.hpp"
#include <core/time.hpp>

#include <resources/material/material.hpp>
#include <resources/image/image_gl.hpp>
#include <resources/mesh/mesh_gl.hpp>

#include <grass/grass_chunk.hpp>
#include "platform/opengl/open_gl.hpp"
#include "platform/opengl/shader_gl.hpp"
#include "wind/wind.hpp"
#include <tools/log.hpp>

#include "platform/opengl/gl_uniform.hpp"
#include "rendering/skybox.hpp"
#include <grass/grass_renderer.hpp>
#include <rendering/post_process/post_process_manager.hpp>
#include "rendering/post_process/post_process_effects.hpp"
#include <platform/opengl/shader_gl.hpp>
#include <terrain/terrain_renderer.hpp>

#include "platform/opengl/uniforms_gl.hpp"
#include "rendering/model_renderer.hpp"
#include "rendering/ibl_renderer.hpp"
#include "rendering/shader_db.hpp"

#define DEBUG_UBO_LOCATION (UBO_LOCATION_COUNT + 1)


class bee::Renderer::Impl
{
public:
    void CreateFrameBuffers();
    void DeleteFrameBuffers();
    void CreateShadowMaps();
    void DeleteShadowMaps();
    void RenderShadowMaps(TerrainRenderer& terrainRenderer, Uniform<TransformsUBO>& instanceBuffer, const std::vector<ObjectInfo>& objectsToDraw, const std::vector<LightInfo>& lightsToDraw, const Camera& camera);

    int m_width = -1;
    int m_height = -1;
    int m_msaa = 4;
    static const int m_maxHDR = 2;
    static const int m_additionalRenderTargetCount = 2;
    
    std::vector<float>  m_shadowCascadeLevels = {};
    const int m_shadowResolution = 2048;
    float m_exposure = 1.0f;

    uint32_t m_hdrFramebuffer = 0;
    std::array<unsigned int, m_maxHDR> m_hdrColorbuffers;
    uint32_t m_hdrWSPositionBuffer = 0;
    uint32_t m_hdrNormalBuffer = 0;
    uint32_t m_hdrDepthbuffer = 0;

    uint32_t m_msaaFramebuffer = 0;
    std::array<unsigned int, m_maxHDR>  m_msaaColorbuffers;
    uint32_t m_msaaWSPositionBuffer = 0;
    uint32_t m_msaaNormalBuffer = 0;
    uint32_t m_msaaDepthbuffer = 0;

    uint32_t m_finalFramebuffer = 0;
    uint32_t m_finalColorbuffer = 0;
    uint32_t m_finalDepthbuffer = 0;

    uint32_t m_hdrTexture = 0;
    // TODO: remove hardcoded value
    std::array<unsigned int, m_maxDirLights * 4> m_shadowFBOs;
    std::array<unsigned int, m_maxDirLights> m_shadowMaps;


    std::shared_ptr<Vignette> m_vignetteProcess = nullptr;
    std::shared_ptr<Bloom> m_bloomProcess = nullptr;
    std::shared_ptr<DepthOfField> m_dofProcess = nullptr;

    std::shared_ptr<Shader> m_envDebugPass = nullptr;
    
    Uniform<DirectionalLightsUBO> m_DirectionalLightUBO;
    Uniform<CameraUBO> m_CameraDataUBO;
    Uniform<PointLightsUBO> m_PointLightUBO;


    bool m_shouldBlit = true;
};


bee::Renderer::Renderer()
{
    m_impl = std::make_unique<Impl>();

    // TODO: Implement this using the HDR from the skybox.
    m_ibl = std::make_unique<IBLRenderer>();

    m_modelRenderer = std::make_unique<ModelRenderer>(m_debugFlags, m_ibl->SpecularMipCount());
    m_grassRenderer = std::make_unique<GrassRenderer>(m_modelRenderer->GetIBL());
    m_terrainRenderer = std::make_unique<TerrainRenderer>(m_debugFlags, m_modelRenderer->GetIBL(), m_ibl->SpecularMipCount());
    m_ui = std::make_unique<UIRenderer>();
    m_postProcessor = std::make_unique<PostProcessManager>();

    
    // Post processes
    m_postProcessor->InsertProcess(PostProcess::Type::Vignette);
    m_postProcessor->InsertProcess(PostProcess::Type::Bloom);
    m_postProcessor->InsertProcess(PostProcess::Type::DepthOfField);
    m_postProcessor->Reorder({ PostProcess::Type::Vignette, PostProcess::Type::DepthOfField, PostProcess::Type::Bloom });


    const float farPlane = 1000;
    m_impl->m_shadowCascadeLevels = {
        farPlane / 50.0f,
        farPlane / 25.0f,
        farPlane / 10.0f,
        farPlane / 2.0f };

    m_impl->CreateFrameBuffers();
    m_impl->CreateShadowMaps();

    glEnable(GL_PROGRAM_POINT_SIZE);

    // Camera UBO
    m_impl->m_CameraDataUBO->bee_eyePos = glm::vec3(1.0f, 2.0f, 3.0f);
    m_impl->m_CameraDataUBO->bee_time = 3.14f;
    m_impl->m_CameraDataUBO->bee_FogColor = glm::vec4(91.0f / 255.0f, 178.0f / 255.0f, 201.0f / 255.0f, 1.0f);
    m_impl->m_CameraDataUBO->bee_FogFar = 256.0f;
    m_impl->m_CameraDataUBO->bee_FogNear = 128.0f;

    m_impl->m_CameraDataUBO.Patch();
    m_impl->m_CameraDataUBO.SetName("Camera UBO (size:" + std::to_string(sizeof(CameraUBO)) + ")");
    glBindBufferBase(GL_UNIFORM_BUFFER, CAMERA_UBO_LOCATION, m_impl->m_CameraDataUBO.buffer);

    //Lights
    m_impl->m_DirectionalLightUBO.SetName("Dir Lights UBO (size:" + std::to_string(sizeof(DirectionalLightsUBO)) + ")");
    glBindBufferBase(GL_UNIFORM_BUFFER, DIRECTIONAL_LIGHTS_UBO_LOCATION, m_impl->m_DirectionalLightUBO.buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHTS_UBO_LOCATION, m_impl->m_PointLightUBO.buffer);

    //Setup shadow map sampler values
    Engine.ShaderDB()[ShaderDB::Type::FORWARD]->Activate();
    for (int i = 0; i < m_maxDirLights; i++)
    {
        int sampler = SHADOWMAP_LOCATION + i;
        glUniform1i(sampler, sampler);
    }

    BEE_DEBUG_ONLY(Engine.ShaderDB()[ShaderDB::Type::TERRAIN]->Deactivate());
}

bee::Renderer::~Renderer()
{
    m_impl->DeleteFrameBuffers();
    m_impl->DeleteShadowMaps();
}

void bee::Renderer::Impl::CreateFrameBuffers()
{
    m_width = Engine.Device().GetWidth();
    m_height = Engine.Device().GetHeight();

    //  -- MSAA framebuffer --
    glGenFramebuffers(1, &m_msaaFramebuffer);              // Create
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFramebuffer);  // Bind FBO
    LabelGL(GL_FRAMEBUFFER, m_msaaFramebuffer, "[R] MSAA Frame Buffer");

    // MSAA color buffers
    glGenTextures(2, &m_msaaColorbuffers[0]);              // Create MSAA color attachments
    for (int i = 0; i < m_maxHDR; i++)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_msaaColorbuffers[i]);  // Bind
        LabelGL(GL_TEXTURE, m_msaaColorbuffers[i], ("[R] MSAA Color Buffer" + std::to_string(i)).c_str());
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa, GL_RGBA16F, m_width, m_height, GL_TRUE);  // Set storage
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D_MULTISAMPLE, m_msaaColorbuffers[i], 0);  // Attach it
    }

    // World space position buffer
    glGenTextures(1, &m_msaaWSPositionBuffer);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_msaaWSPositionBuffer);
    LabelGL(GL_TEXTURE, m_msaaWSPositionBuffer, "[R] MSAA WSPosition Buffer");
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa, GL_RGBA32F, m_width, m_height, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + m_maxHDR, GL_TEXTURE_2D_MULTISAMPLE, m_msaaWSPositionBuffer, 0);

    // Normal buffer
    glGenTextures(1, &m_msaaNormalBuffer);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_msaaNormalBuffer);
    LabelGL(GL_TEXTURE, m_msaaNormalBuffer, "[R] MSAA Normal Buffer");
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa, GL_RGB32F, m_width, m_height, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (m_maxHDR + 1), GL_TEXTURE_2D_MULTISAMPLE, m_msaaNormalBuffer, 0);

    // MSAA depth buffer
    glGenRenderbuffers(1, &m_msaaDepthbuffer);               // Create
    glBindRenderbuffer(GL_RENDERBUFFER, m_msaaDepthbuffer);  // Bind
    LabelGL(GL_RENDERBUFFER, m_msaaDepthbuffer, "[R] MSAA Depth Buffer");
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_msaa, GL_DEPTH_COMPONENT, m_width, m_height);    // Set storage
    glBindRenderbuffer(GL_RENDERBUFFER, 0);                                                              // Unbind
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_msaaDepthbuffer);  // Attach it

    // Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int msaa_attachments[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, msaa_attachments);

    // Check that our framebuffer is OK
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) assert(false);
    BEE_DEBUG_ONLY(glBindFramebuffer(GL_FRAMEBUFFER, 0));


    // -- HDR framebuffer --
    glGenFramebuffers(1, &m_hdrFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFramebuffer);
    LabelGL(GL_FRAMEBUFFER, m_hdrFramebuffer, "[R] HDR Frame Buffer");

    // HDR color buffers
    glGenTextures(2, &m_hdrColorbuffers[0]);
    for (int i = 0; i < m_maxHDR; i++)
    {
        glBindTexture(GL_TEXTURE_2D, m_hdrColorbuffers[i]);
        LabelGL(GL_TEXTURE, m_hdrColorbuffers[i], ("[R] HDR Color Buffer" + std::to_string(i)).c_str());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_hdrColorbuffers[i], 0);
    }

    // HDR world space position
    glGenTextures(1, &m_hdrWSPositionBuffer);
    glBindTexture(GL_TEXTURE_2D, m_hdrWSPositionBuffer);
    LabelGL(GL_TEXTURE, m_hdrWSPositionBuffer, "[R] HDR WSPosition Buffer");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + m_maxHDR, GL_TEXTURE_2D, m_hdrWSPositionBuffer, 0);

    glGenTextures(1, &m_hdrNormalBuffer);
    glBindTexture(GL_TEXTURE_2D, m_hdrNormalBuffer);
    LabelGL(GL_TEXTURE, m_hdrNormalBuffer, "[R] HDR Normal Buffer");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (m_maxHDR + 1), GL_TEXTURE_2D, m_hdrNormalBuffer, 0);

    // HDR depth buffer
    glGenRenderbuffers(1, &m_hdrDepthbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_hdrDepthbuffer);
    LabelGL(GL_RENDERBUFFER, m_hdrDepthbuffer, "[R] HDR Depth Buffer");
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFramebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_hdrDepthbuffer);

    unsigned int hdr_attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, hdr_attachments);

    // Check that our framebuffer is OK
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) assert(false);
    BEE_DEBUG_ONLY(glBindFramebuffer(GL_FRAMEBUFFER, 0));


    // -- Final framebuffer --
    glGenFramebuffers(1, &m_finalFramebuffer);              // Create
    glBindFramebuffer(GL_FRAMEBUFFER, m_finalFramebuffer);  // Bind FBO
    LabelGL(GL_FRAMEBUFFER, m_finalFramebuffer, "[R] Final Frame Buffer");

    // Color buffer
    glGenTextures(1, &m_finalColorbuffer);             // Resolved
    glBindTexture(GL_TEXTURE_2D, m_finalColorbuffer);  // Bind
    LabelGL(GL_TEXTURE, m_finalColorbuffer, "[R] Final Color Buffer");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);        // Set storage
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);                                    // Filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                                    // Filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);                                 // Clamping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);                                 // Clamping
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_finalColorbuffer, 0);  // Attach it

    unsigned int finalAttachments[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, finalAttachments);

    // Check that our framebuffer is OK
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) assert(false);
    BEE_DEBUG_ONLY(glBindFramebuffer(GL_FRAMEBUFFER, 0));

}

void bee::Renderer::Impl::DeleteFrameBuffers()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteTextures(2, &m_msaaColorbuffers[0]);
    glDeleteRenderbuffers(1, &m_msaaDepthbuffer);
    glDeleteFramebuffers(1, &m_msaaFramebuffer);

    glDeleteTextures(2, &m_hdrColorbuffers[0]);
    glDeleteRenderbuffers(1, &m_hdrDepthbuffer);
    glDeleteFramebuffers(1, &m_hdrFramebuffer);

    glDeleteTextures(1, &m_finalColorbuffer);
    glDeleteRenderbuffers(1, &m_hdrFramebuffer);
}

void bee::Renderer::Impl::DeleteShadowMaps()
{
    glDeleteTextures(6, m_shadowMaps.data());
    glDeleteFramebuffers(6, m_shadowFBOs.data());
}

void bee::Renderer::SetAmbientFactor(float ambientFactor) {
    m_impl->m_CameraDataUBO->bee_ambientFactor = glm::clamp(ambientFactor, 0.0f, 1.0f);
    m_impl->m_CameraDataUBO.Patch();
}

void bee::Renderer::QueueMesh(const glm::mat4& transform, ResourceHandle<Mesh> mesh, ResourceHandle<Material> material, MeshRenderer* meshRenderer)
{
    //Avoid invalid handles from being submitted
    if (auto mesh_ptr = mesh.Retrieve()) if (auto mat_ptr = material.Retrieve()) {
        m_objectsToDraw.emplace_back(
            ObjectInfo { transform, mesh_ptr, mat_ptr, meshRenderer }
        );
    }
}

void bee::Renderer::QueueLight(const glm::mat4& transform, const Light& light)
{
    m_lightsToDraw.emplace_back(
        LightInfo { transform, light }
    );
}

void bee::Renderer::SetFog(glm::vec4 fogColor, float fogNear, float fogFar)
{
    m_impl->m_CameraDataUBO->bee_FogNear = fogNear;
    m_impl->m_CameraDataUBO->bee_FogFar = fogFar;
    m_impl->m_CameraDataUBO->bee_FogColor = fogColor;
    m_impl->m_CameraDataUBO.Patch();
}

void bee::Renderer::SetSkybox(ResourceHandle<Image> skyboxImage)
{
    m_skybox = std::make_unique<Skybox>(skyboxImage, Engine.ShaderDB()[ShaderDB::Type::SKYBOX]);
    m_ibl->Render(m_skybox->GetSkyboxCubemap(), m_modelRenderer->GetIBL());
}

void bee::Renderer::Impl::CreateShadowMaps()
{
    for (int i = 0; i < m_maxDirLights; i++)
    {
        const int size = m_shadowResolution;

        // Shadows being made
        glGenTextures(1, &m_shadowMaps[i]);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowMaps[i]);
        LabelGL(GL_TEXTURE, m_shadowMaps[i], ("[R] Shadow Map" + std::to_string(i)).c_str());

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, size, size, int(m_shadowCascadeLevels.size()) + 1, 
            0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float borderColor[] = {1.0, 1.0, 1.0, 1.0};
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

        auto numOfCascades = m_shadowCascadeLevels.size();
        for (size_t j = 0; j < numOfCascades; j++)
        {
            glGenFramebuffers(1, &m_shadowFBOs[i * numOfCascades + j]);
            glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBOs[i * numOfCascades + j]);
            LabelGL(GL_FRAMEBUFFER, m_shadowFBOs[i * numOfCascades + j], ("[R] Shadow Map FBO" + std::to_string(i * numOfCascades + j)).c_str());
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_shadowMaps[i], 0, j);
            // Check that our framebuffer is OK
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) assert(false);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void bee::Renderer::Impl::RenderShadowMaps(TerrainRenderer& terrainRenderer, Uniform<TransformsUBO>& instanceBuffer, 
    const std::vector<ObjectInfo>& objectsToDraw, 
    const std::vector<LightInfo>& lightsToDraw, const Camera& camera)
{
 
    uint32_t lightIndex = 0;
    for (auto& entry : lightsToDraw) {
        std::string debugLabel = "Shadow pass #" + std::to_string(lightIndex);
        PushDebugGL(debugLabel);

        //Early out
        if (lightIndex >= MAX_DIRECTIONAL_LIGHTS) {
            Log::Warn("Max level of directional lights reached");
            break;
        }

        //Early out
        if (entry.light.Type != Light::Type::Directional || entry.light.CastShadows == false) 
            continue;

        //Area that the shadow map covers
        glm::vec3 lightDir = entry.transform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        const auto lightMatrices = GetLightSpaceMatrices(camera, lightDir, m_shadowCascadeLevels);

        auto numOfCascades = m_shadowCascadeLevels.size();
        for (size_t i = 0; i < numOfCascades; i++)
        {
            m_CameraDataUBO->bee_viewProjection = lightMatrices[i];
            m_CameraDataUBO.Patch();

            Engine.ShaderDB()[ShaderDB::Type::SHADOW]->Activate();
            glViewport(0, 0, m_shadowResolution, m_shadowResolution);
            glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBOs[lightIndex * numOfCascades + i]);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_DEPTH_BUFFER_BIT);
            glCullFace(GL_FRONT);
            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);

            //Traverse the list, collecting transforms and instancing all meshes
            size_t draw_ptr = 0;
            while (draw_ptr < objectsToDraw.size()) {

                auto batch_mesh = objectsToDraw.at(draw_ptr).mesh;

                size_t instanceCount = 0;
                for (size_t lookPtr = draw_ptr; lookPtr < objectsToDraw.size() && instanceCount < MAX_TRANSFORM_INSTANCES; ++lookPtr) {
                    auto& nextElement = objectsToDraw.at(lookPtr);

                    if (nextElement.mesh != batch_mesh) break;

                    instanceBuffer->bee_transforms[instanceCount].world = nextElement.transform;
                    instanceCount++;
                }

                
                // Render instances.
                instanceBuffer.Patch();

                Material::ApplyAlbedo(objectsToDraw.at(draw_ptr).material);

                glBindVertexArray(batch_mesh->vao_handle);
                glDrawElementsInstanced(GL_TRIANGLES, batch_mesh->index_count, batch_mesh->index_format, nullptr, static_cast<GLsizei>(instanceCount));

                draw_ptr += instanceCount;
            }

            terrainRenderer.DepthOnlyRender(Engine.ShaderDB()[ShaderDB::Type::TERRAIN_SHADOW]);
        }
        lightIndex++;
        PopDebugGL();
    }
    glCullFace(GL_BACK);
}

void* bee::Renderer::GetOutputFramebuffer()
{
    return &m_impl->m_finalColorbuffer;
}

void bee::Renderer::SetScreenBlit(bool value)
{
    m_impl->m_shouldBlit = value;
}

void bee::Renderer::Render()
{

    //Pick the first camera (TODO: add option to set a camera or a camera entity)
    auto cameraView = Engine.ECS().Registry.view<Transform, CameraComponent>();

    //TODO set default value if no camera exists
    
    Camera frameCamera{};

    if (cameraView.begin() != cameraView.end()) 
    {
        auto cameraTransform = Engine.ECS().Registry.get<Transform>(cameraView.front()).World();
        auto& cameraComponent = Engine.ECS().Registry.get<CameraComponent>(cameraView.front());

        if (cameraComponent.isOrthographic) {
            frameCamera = Camera::Orthographic(
                cameraTransform[3],
                cameraTransform[3] + cameraTransform * glm::vec4(World::FORWARD, 0.0f),
                cameraComponent.aspectRatio,
                cameraComponent.fieldOfView * 100.0f,
                cameraComponent.nearClip,
                cameraComponent.farClip
            );
        }
        else {
             frameCamera = Camera::Perspective(
                cameraTransform[3],
                cameraTransform[3] + cameraTransform * glm::vec4(World::FORWARD, 0.0f),
                cameraComponent.aspectRatio,
                cameraComponent.fieldOfView,
                cameraComponent.nearClip,
                cameraComponent.farClip
            );
        }
    }
    else 
    {
        Log::Warn("RENDERER: No camera exists in the scene to render from.");
    }

    const glm::mat4 view = frameCamera.GetView();
    const glm::mat4 projection = frameCamera.GetProjection();
    const glm::vec4 eyePos = glm::vec4(frameCamera.GetPosition(), 1.0f);

    // 1. Sort objects based on material (required for instancing)
    std::sort(m_objectsToDraw.begin(), m_objectsToDraw.end(),
        [](const ObjectInfo& lhs, const ObjectInfo& rhs) {
            if (lhs.material.get() == rhs.material.get()) return lhs.mesh.get() < rhs.mesh.get();
            return lhs.material.get() < rhs.material.get();
        }
    );

    // 2. Render to shadow maps
    // TODO: Find better way to communicate instance buffer.
    m_impl->RenderShadowMaps(*m_terrainRenderer, *static_cast<Uniform<TransformsUBO>*>(m_modelRenderer->InstancedTransformBuffer()), m_objectsToDraw, m_lightsToDraw, frameCamera);

    //MSAA Framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_msaaFramebuffer);

    PushDebugGL("Clear pass");
    glViewport(0, 0, m_impl->m_width, m_impl->m_height);
    glClearColor(0.5f, 0.8f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    PopDebugGL();

    glEnable(GL_CULL_FACE);

    size_t dir_lights = 0;
    size_t point_lights = 0;

    // 3. Update light and shadow matrix uniforms
    for (auto& entry : m_lightsToDraw) {

        switch (entry.light.Type)
        {
        case Light::Type::Directional:
            {

            if (dir_lights >= MAX_DIRECTIONAL_LIGHTS) {
                Log::Warn("Exceeding max directional light uniforms");
                break;
            }

            auto& uniform_index = m_impl->m_DirectionalLightUBO->bee_directional_lights[dir_lights];
            auto shadow_size = entry.light.ShadowExtent;

            glm::vec3 lightDir = entry.transform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
            uniform_index.color = entry.light.Color;
            uniform_index.direction = lightDir;
            uniform_index.intensity = entry.light.Intensity;

            auto cascadeMatrices = GetLightSpaceMatrices(frameCamera, lightDir, m_impl->m_shadowCascadeLevels);
            for (size_t i = 0; i < m_impl->m_shadowCascadeLevels.size(); i++)
            {
                uniform_index.shadow_matrices[i] = cascadeMatrices[i];
            }

            dir_lights++;
            }
            break;
        case Light::Type::Point:
            {

            if (point_lights++ > MAX_POINT_LIGHT_INSTANCES) {
                Log::Warn("Exceeding max point light uniforms");
                break;
            }

            auto& uniform_index = m_impl->m_PointLightUBO->bee_point_lights[point_lights];

            uniform_index.color = entry.light.Color;
            uniform_index.intensity = entry.light.Intensity;
            uniform_index.position = entry.transform[3];
            uniform_index.range = entry.light.Range;

            point_lights++;
            }
            break;
        default:
            Log::Warn("Rendering detected an unsupported light type");
            break;
        }
    }

    m_impl->m_PointLightUBO.Patch();
    m_impl->m_DirectionalLightUBO.Patch();

    m_impl->m_PointLightUBO.Patch();
    m_impl->m_DirectionalLightUBO.Patch();

    //Camera UBO

    auto seconds = Engine.GetTime().GetTotalTime().count() / 1000.0f;

    m_impl->m_CameraDataUBO->bee_time = seconds;
    m_impl->m_CameraDataUBO->bee_view = view;
    m_impl->m_CameraDataUBO->bee_projection = projection;
    m_impl->m_CameraDataUBO->bee_viewProjection = projection * view;
    m_impl->m_CameraDataUBO->bee_eyePos = eyePos;
    m_impl->m_CameraDataUBO->bee_directionalLightsCount = static_cast<int>(dir_lights);
    m_impl->m_CameraDataUBO->bee_pointLightsCount = static_cast<int>(point_lights);
    m_impl->m_CameraDataUBO->bee_resolution = glm::vec2(static_cast<float>(Engine.Device().GetWidth()), 
                                                        static_cast<float>(Engine.Device().GetHeight()));
    m_impl->m_CameraDataUBO->bee_cascadeCount = m_impl->m_shadowCascadeLevels.size();
    for (size_t i = 0; i < m_impl->m_shadowCascadeLevels.size(); i++)
    {
        m_impl->m_CameraDataUBO->bee_cascadePlaneDistances[i].x = m_impl->m_shadowCascadeLevels.at(i);
    }
    m_impl->m_CameraDataUBO->bee_farPlane = frameCamera.GetFarPlane();

    m_impl->m_CameraDataUBO.Patch();

    // 5. Bind shadow map textures(should be done explicitly per pass)
    // TODO: should be done manually per pass, instead of relaying on this.
    for (int i = 0; i < m_maxDirLights; ++i)
    {
        int sampler = SHADOWMAP_LOCATION + i;
        glActiveTexture(GL_TEXTURE0 + sampler);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_impl->m_shadowMaps[i]);
    }

    // 6. Render skybox
    if (m_skybox) m_skybox->Render();

    // 7. Render terrain
    m_terrainRenderer->Render();

    // 8. Render grass
    m_grassRenderer->Render();

    //Object frustum culling

    auto frustumPlanes = frameCamera.GetFrustum();

    std::vector<ObjectInfo> visibleObjects; 
    visibleObjects.reserve(m_objectsToDraw.size());

    for (auto& object : m_objectsToDraw)
    {
        auto aabb = object.mesh->bounds.ApplyTransform(object.transform);
        if (aabb.FrustumTest(frustumPlanes))
        {
            visibleObjects.push_back(object);
        }
    }

    m_objectsToDraw = visibleObjects;

    // 9. Render standard models.
    m_modelRenderer->Render(m_objectsToDraw, m_lightsToDraw);
    m_objectsToDraw.clear();
    m_lightsToDraw.clear();

    // 10. Resolve MSAA into HDR
    PushDebugGL("Resolve MSAA");
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_impl->m_msaaFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_impl->m_hdrFramebuffer);
    for (int i = 0; i < m_impl->m_maxHDR + m_impl->m_additionalRenderTargetCount; i++)
    {
        glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
        glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
        glBlitFramebuffer(0, 0, m_impl->m_width, m_impl->m_height, 0, 0, m_impl->m_width, m_impl->m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    PopDebugGL();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // 11. Perform post processes
    PostProcess::RenderTextureCollection rtCollection{};
    rtCollection.sourceTexture = &m_impl->m_hdrColorbuffers[0];
    rtCollection.brightnessTexture = &m_impl->m_hdrColorbuffers[1];
    rtCollection.normalTexture = &m_impl->m_hdrNormalBuffer;
    rtCollection.worldSpacePositionTexture = &m_impl->m_hdrWSPositionBuffer;

    m_postProcessor->Draw(rtCollection, &m_impl->m_hdrFramebuffer);

    // 12. ui
    m_ui->Render();

    // 13. Tonemap HDR into LDR
    PushDebugGL("Tone mapping pass");
    glBindFramebuffer(GL_FRAMEBUFFER, m_impl->m_finalFramebuffer);
    glViewport(0, 0, m_impl->m_width, m_impl->m_height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_impl->m_hdrColorbuffers[0]);

    Engine.ShaderDB()[ShaderDB::Type::TONEMAPPING]->Activate();
    Engine.ShaderDB()[ShaderDB::Type::TONEMAPPING]->GetParameter("u_hdrBuffer")->SetValue(0);
    Engine.ShaderDB()[ShaderDB::Type::TONEMAPPING]->GetParameter("u_exposure")->SetValue(m_impl->m_exposure);
    bee::RenderQuad();
    PopDebugGL();

    // 14. Blit result to screen
    if (m_impl->m_shouldBlit)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_impl->m_finalFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glDrawBuffer(GL_BACK);
        glBlitFramebuffer(0, 0, m_impl->m_width, m_impl->m_height, 0, 0, m_impl->m_width, m_impl->m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
}
