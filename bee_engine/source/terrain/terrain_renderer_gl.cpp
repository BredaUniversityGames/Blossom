#include <precompiled/engine_precompiled.hpp>
#include "terrain/terrain_renderer.hpp"

#include "rendering/render.hpp"
#include "terrain/terrain_chunk.hpp"
#include "core/engine.hpp"
#include "core/ecs.hpp"
#include <core/transform.hpp>
#include "rendering/render_components.hpp"
#include "platform/opengl/shader_gl.hpp"
#include "core/engine.hpp"
#include "resources/resource_manager.hpp"
#include "resources/mesh/mesh_gl.hpp"
#include <platform/opengl/open_gl.hpp>
#include <resources/image/image_gl.hpp>
#include "platform/opengl/uniforms_gl.hpp"
#include "rendering/shader_db.hpp"

class bee::TerrainRenderer::Impl
{
public:
    int SamplerTypeToGL(Sampler::Filter filter);
    int SamplerTypeToGL(Sampler::Wrap wrap);
};

int bee::TerrainRenderer::Impl::SamplerTypeToGL(bee::Sampler::Filter filter)
{
    switch (filter)
    {
        case Sampler::Filter::Nearest:
            return GL_NEAREST;
        case Sampler::Filter::Linear:
            return GL_LINEAR;
        case Sampler::Filter::NearestMipmapNearest:
            return GL_NEAREST_MIPMAP_NEAREST;
        case Sampler::Filter::LinearMipmapNearest:
            return GL_LINEAR_MIPMAP_NEAREST;
        case Sampler::Filter::NearestMipmapLinear:
            return GL_NEAREST_MIPMAP_LINEAR;
        case Sampler::Filter::LinearMipmapLinear:
            return GL_LINEAR_MIPMAP_LINEAR;
    }
    return 0;
}

int bee::TerrainRenderer::Impl::SamplerTypeToGL(bee::Sampler::Wrap wrap)
{
    switch (wrap)
    {
        case Sampler::Wrap::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
        case Sampler::Wrap::MirroredRepeat:
            return GL_MIRRORED_REPEAT;
        case Sampler::Wrap::Repeat:
            return GL_REPEAT;
    }
    return 0;
}

bee::TerrainRenderer::TerrainRenderer(const DebugData& debugFlags, const Material::IBL& ibl, uint32_t iblSpecularMipCount) 
    : m_impl(std::make_unique<Impl>()), m_debugFlags(debugFlags), m_ibl(ibl), m_iblSpecularMipCount(iblSpecularMipCount)
{
    m_terrainPass = Engine.ShaderDB()[ShaderDB::Type::TERRAIN];
}

bee::TerrainRenderer::~TerrainRenderer() { }

void bee::TerrainRenderer::Submit(entt::entity entity) { }

void bee::TerrainRenderer::CleanUp(entt::entity entity) { }

void bee::TerrainRenderer::Render()
{
    PushDebugGL("Terrain pass");

    m_terrainPass->Activate();

    for (int i = 0; i < Renderer::m_maxDirLights; i++)
    {
        int sampler = SHADOWMAP_LOCATION + i;
        glUniform1i(sampler, sampler);
    }

    // Create view of all chunks.
    auto terrainChunkView = bee::Engine.ECS().Registry.view <const TerrainChunk, const bee::Transform, const bee::MeshRenderer>();

    //Pick the first camera (TODO: add option to set a camera or a camera entity)
    auto camera_view = Engine.ECS().Registry.view<Transform, CameraComponent>();
    glm::mat4 camera_transform{};
    if (camera_view.begin() != camera_view.end())
        camera_transform = Engine.ECS().Registry.get<Transform>(camera_view.front()).World();
    glm::vec4 eyePos = camera_transform[3];

    glPatchParameteri(GL_PATCH_VERTICES, 3);

    // Iterate view for rendering.
    for (auto [e, chunk, transform, renderer] : terrainChunkView.each())
    {
        if (!chunk.heightmap.Valid()) continue;

        glBindVertexArray(renderer.GetMesh().Retrieve()->vao_handle);

        glm::mat4 world = transform.World();
        uint32_t indexCount = renderer.GetMesh().Retrieve()->index_count;

        // Bind heightmap
        glActiveTexture(GL_TEXTURE0 + 16);
        glBindTexture(GL_TEXTURE_2D, chunk.heightmap.Retrieve()->handle);
        glUniform1i(m_terrainPass->GetParameter("s_heightmap")->GetLocation(), 16);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_impl->SamplerTypeToGL(Sampler::Filter::Linear));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_impl->SamplerTypeToGL(Sampler::Filter::Linear));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_impl->SamplerTypeToGL(Sampler::Wrap::ClampToEdge));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_impl->SamplerTypeToGL(Sampler::Wrap::ClampToEdge));

        // Tessellation control shader
        m_terrainPass->GetParameter("u_tessDist")->SetValue(chunk.tesselationDistance);
        m_terrainPass->GetParameter("u_tessFactorMax")->SetValue(chunk.tesselationFactor);
        m_terrainPass->GetParameter("u_normalScale")->SetValue(chunk.normalScale);
        m_terrainPass->GetParameter("u_terrainSize")->SetValue(glm::vec2(chunk.width, chunk.height));
        m_terrainPass->GetParameter("u_eyePos")->SetValue(eyePos);
        
        // Tessellation evaluation shader
        m_terrainPass->GetParameter("u_model")->SetValue(world);
        m_terrainPass->GetParameter("u_heightModifier")->SetValue(chunk.heightModifier);

        m_terrainPass->GetParameter("u_tiling")->SetValue(glm::vec2{std::max(chunk.width, chunk.height) * 0.5f});

        Material::Apply(renderer.Material.Retrieve(), m_terrainPass, m_debugFlags, m_ibl, m_iblSpecularMipCount);

        glDrawElements(GL_PATCHES, indexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    m_terrainPass->Deactivate();

    PopDebugGL();
}

void bee::TerrainRenderer::DepthOnlyRender(std::shared_ptr<bee::Shader> depthOnlyShader)
{
    depthOnlyShader->Activate();

    // Create view of all chunks.
    auto terrainChunkView = bee::Engine.ECS().Registry.view <const TerrainChunk, const bee::Transform, const bee::MeshRenderer>();

    glPatchParameteri(GL_PATCH_VERTICES, 3);

    //Pick the first camera (TODO: add option to set a camera or a camera entity)
    auto camera_view = Engine.ECS().Registry.view<Transform, CameraComponent>();
    glm::mat4 camera_transform{};
    if (camera_view.begin() != camera_view.end())
        camera_transform = Engine.ECS().Registry.get<Transform>(camera_view.front()).World();
    glm::vec4 eyePos = camera_transform[3];

    // Iterate view for rendering.
    for (auto [e, chunk, transform, renderer] : terrainChunkView.each())
    {
        if (!chunk.heightmap.Valid()) continue;

        glBindVertexArray(renderer.GetMesh().Retrieve()->vao_handle);

        glm::mat4 world = transform.World();
        uint32_t indexCount = renderer.GetMesh().Retrieve()->index_count;

        // Bind heightmap
        glActiveTexture(GL_TEXTURE0 + 16);
        glBindTexture(GL_TEXTURE_2D, chunk.heightmap.Retrieve()->handle);
        glUniform1i(depthOnlyShader->GetParameter("s_heightmap")->GetLocation(), 16);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_impl->SamplerTypeToGL(Sampler::Filter::Linear));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_impl->SamplerTypeToGL(Sampler::Filter::Linear));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_impl->SamplerTypeToGL(Sampler::Wrap::ClampToEdge));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_impl->SamplerTypeToGL(Sampler::Wrap::ClampToEdge));

        // Tessellation control shader
        depthOnlyShader->GetParameter("u_tessDist")->SetValue(chunk.tesselationDistance);
        depthOnlyShader->GetParameter("u_tessFactorMax")->SetValue(chunk.tesselationFactor);
        depthOnlyShader->GetParameter("u_eyePos")->SetValue(eyePos);

        // Tessellation evaluation shader
        depthOnlyShader->GetParameter("u_model")->SetValue(world);
        depthOnlyShader->GetParameter("u_heightModifier")->SetValue(chunk.heightModifier);

        glDrawElements(GL_PATCHES, indexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    depthOnlyShader->Deactivate();
}
