#include <precompiled/engine_precompiled.hpp>
#include "grass/grass_renderer.hpp"

#include "core/engine.hpp"
#include <platform/opengl/open_gl.hpp>
#include "core/ecs.hpp"
#include "grass/grass_chunk.hpp"
#include <core/transform.hpp>
#include "rendering/render_components.hpp"
#include "platform/opengl/shader_gl.hpp"
#include "core/engine.hpp"
#include "resources/resource_manager.hpp"
#include "wind/wind.hpp"
#include "../assets/shaders/grass/grass_locations.glsl"
#include "../assets/shaders/locations.glsl"
#include "displacement/displacement_manager.hpp"
#include "rendering/render.hpp"
#include "rendering/shader_db.hpp"
#include "resources/mesh/mesh_gl.hpp"
#include "platform/opengl/gl_uniform.hpp"

#include "terrain/terrain_chunk.hpp"
#include "tools/log.hpp"

class bee::GrassRenderer::Impl
{
public:

    GLuint m_grassBladeVBO;
    GLuint m_grassBladeVAO;
    std::array<std::pair<GLuint, GLuint>, 3> m_LODIndex;
    std::array<GLuint, 3> m_SSBOs;

    Uniform<GrassChunkMaterial> m_materialBuffer;

    void CreateGrassBladeGeom();
};

bee::GrassRenderer::GrassRenderer(const Material::IBL& ibl) : m_impl(std::make_unique<Impl>()), m_ibl(ibl)
{
    m_grassPass = Engine.ShaderDB()[ShaderDB::Type::GRASS];
    m_grassCompute = Engine.ShaderDB()[ShaderDB::Type::GRASS_COMPUTE];
    m_noiseImage = Engine.Resources().Images().FromFile(FileIO::Directory::Asset, "textures/noiseTexture.png", ImageFormat::RGBA8);
    m_windNoiseImage = Engine.Resources().Images().FromFile(FileIO::Directory::Asset, "textures/wind_noise.png", ImageFormat::RGBA8);

    m_maps.m_colorMap = Engine.Resources().Images().FromFile(FileIO::Directory::Asset, "textures/terrain/Grass_Dense_Tint_01_Base_Basecolor_A.png", ImageFormat::RGBA8);
    m_maps.m_lengthMap = Engine.Resources().Images().FromFile(FileIO::Directory::Asset, "textures/noise/gradient.png", ImageFormat::RGBA8);

    glBindBufferBase(GL_UNIFORM_BUFFER, GRASS_MATERIAL_LOCATION, m_impl->m_materialBuffer.buffer);

    m_impl->CreateGrassBladeGeom();

    bee::Engine.ECS().Registry.on_construct<GrassChunk>().connect<&GrassRenderer::OnGrassChunkCreate>(*this);

    struct
    {
        glm::vec4 position;
        glm::vec2 terrainUV;
        glm::vec2 _padding;
    } instanceData;

    // Set up noise texture for compute
    glActiveTexture(GL_TEXTURE0 + NOISE_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, m_noiseImage.Retrieve()->handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    for(size_t i = 0; i < m_impl->m_SSBOs.size(); ++i)
    {
        const uint32_t density = 16 >> i;
        uint32_t grassCount = density * density * GRASS_PATCH * GRASS_PATCH;

        glGenBuffers(1, &m_impl->m_SSBOs[i]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_impl->m_SSBOs[i]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_impl->m_SSBOs[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(instanceData) * grassCount, nullptr,
                     GL_STATIC_DRAW);


        glBindImageTexture(NOISE_TEXTURE_UNIT, m_noiseImage.Retrieve()->handle, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

        m_grassCompute->Activate();
        glDispatchCompute(density, density, 1);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

}

bee::GrassRenderer::~GrassRenderer()
{
    glDeleteBuffers(1, &m_impl->m_grassBladeVBO);
    glDeleteVertexArrays(1, &m_impl->m_grassBladeVAO);
    for(size_t i = 0; i < m_impl->m_SSBOs.size(); ++i)
        glDeleteBuffers(1, &m_impl->m_SSBOs[i]);

    bee::Engine.ECS().Registry.on_construct<GrassChunk>().disconnect<&GrassRenderer::OnGrassChunkCreate>(*this);
}

void bee::GrassRenderer::OnGrassChunkCreate(entt::registry& registry, entt::entity entity)
{
}

void bee::GrassRenderer::OnGrassChunkDestroy(entt::registry& registry, entt::entity entity)
{ 
}

void bee::GrassRenderer::Render()
{
    PushDebugGL("Grass pass");

    // Create view of all chunks.
    auto grassChunkView = bee::Engine.ECS().Registry.view<const GrassChunk, const bee::Transform>(entt::exclude<CulledGrass>);

    m_grassPass->Activate();

    glDisable(GL_CULL_FACE);

    glUniform1i(m_grassPass->GetParameter("u_noise")->GetLocation(), NOISE_TEXTURE_UNIT);
    glActiveTexture(GL_TEXTURE0 + NOISE_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, m_noiseImage.Retrieve()->handle);

    glUniform1i(m_grassPass->GetParameter("u_windNoise")->GetLocation(), WIND_NOISE_TEXTURE_UNIT);
    glActiveTexture(GL_TEXTURE0 + WIND_NOISE_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_3D, Engine.GetWindMap().GetWindImage().Retrieve()->handle);

    // Set height map texture unit.
    glUniform1i(m_grassPass->GetParameter("u_heightMap")->GetLocation(), HEIGHT_MAP_TEXTURE_UNIT);

    glUniform1i(m_grassPass->GetParameter("u_colorMap")->GetLocation(), COLOR_MAP_TEXTURE_UNIT);
    glActiveTexture(GL_TEXTURE0 + COLOR_MAP_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, m_maps.m_colorMap.Retrieve()->handle);

    glUniform1i(m_grassPass->GetParameter("u_lengthMap")->GetLocation(), LENGTH_MAP_TEXTURE_UNIT);
    glActiveTexture(GL_TEXTURE0 + LENGTH_MAP_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, m_maps.m_lengthMap.Retrieve()->handle);

    glUniform1i(m_grassPass->GetParameter("u_displacementMap")->GetLocation(), DISPLACEMENT_MAP_TEXTURE_UNIT);
    glActiveTexture(GL_TEXTURE0 + DISPLACEMENT_MAP_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, Engine.DisplacementManager().GetTex().Retrieve()->handle);

    for (GLint i = 0; i < Renderer::m_maxDirLights; ++i)
        glUniform1i(SHADOWMAP_LOCATION + i, SHADOWMAP_LOCATION + i);

    auto terrainView = Engine.ECS().Registry.view<TerrainChunk, Transform>();
    auto [terrainChunk, transform] = terrainView.get(*terrainView.begin());

    glm::mat4 terrainTransformMatrix = glm::identity<glm::mat4>();
    terrainTransformMatrix *= glm::translate(glm::identity<glm::mat4>(), glm::vec3{ 0.5f, 0.5f, 0.0f }); // Translate by half to center
    terrainTransformMatrix *= glm::scale(glm::identity<glm::mat4>(), 1.0f / glm::vec3{ terrainChunk.width, terrainChunk.height, 1.0f }); // Scale by terrain size.
    terrainTransformMatrix *= glm::inverse(transform.World());

    glm::mat4 displacementTransformMatrix{ Engine.DisplacementManager().DisplacementMapTransform() };

    m_grassPass->GetParameter("u_terrainTransform")->SetValue(terrainTransformMatrix);
    m_grassPass->GetParameter("u_displacementTransform")->SetValue(displacementTransformMatrix);
    m_grassPass->GetParameter("u_heightModifier")->SetValue(terrainChunk.heightModifier);
    m_grassPass->GetParameter("u_tiling")->SetValue(glm::vec2{ std::max(terrainChunk.width, terrainChunk.height) * 0.5f });
    m_grassPass->GetParameter("u_dither_distance")->SetValue(Engine.Renderer().GetDitherDistance());

    glBindVertexArray(m_impl->m_grassBladeVAO);

    //Pick the first camera (TODO: add option to set a camera or a camera entity)
    auto cameraView = Engine.ECS().Registry.view<Transform, CameraComponent>();

    std::vector<std::tuple<const GrassChunk*, const Transform*>> chunks{};
    for (auto grassChunkEntity : grassChunkView)
    {
        auto pair = grassChunkView[grassChunkEntity];
        chunks.emplace_back(&std::get<0>(pair), &std::get<1>(pair));
    }
    auto cameraTransform = Engine.ECS().Registry.get<Transform>(cameraView.front());

    // Sort based on distance to the camera
    std::sort(chunks.begin(), chunks.end(), [cameraTransform](const std::tuple<const GrassChunk*, const Transform*>& a, const std::tuple<const GrassChunk*, const Transform*>& b)
        {
            glm::vec3 positionCamera = cameraTransform.GetTranslation();
            float distanceA = glm::distance(positionCamera, std::get<1>(a)->GetTranslation());
            float distanceB = glm::distance(positionCamera, std::get<1>(b)->GetTranslation());
            return distanceA < distanceB;
        });


    // Iterate view for rendering.
    for (auto grassChunkEntity : chunks)
    {
        // Grab data from entity and draw to screen.
        auto [chunk, transform] = grassChunkEntity;

        glm::mat4 world = transform->World();

        if (chunk->heightMapImage.Valid())
        {
            glActiveTexture(GL_TEXTURE0 + HEIGHT_MAP_TEXTURE_UNIT);

            glBindTexture(GL_TEXTURE_2D, chunk->heightMapImage.Retrieve()->handle);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        }

        glActiveTexture(GL_TEXTURE0 + SPECULAR_SAMPER_LOCATION);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_ibl.specular->handle);
        glUniform1i(SPECULAR_SAMPER_LOCATION, SPECULAR_SAMPER_LOCATION);


        uint32_t validIndex = std::clamp(chunk->lod, 0u, static_cast<uint32_t>(m_impl->m_LODIndex.size() - 1));
        auto indexRange = m_impl->m_LODIndex.at(validIndex);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_impl->m_SSBOs[validIndex]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_impl->m_SSBOs[validIndex]);


        m_grassPass->GetParameter("u_world")->SetValue(world);
        m_grassPass->GetParameter("u_lodLevel")->SetValue(chunk->lod);


        const uint32_t density = 16 >> validIndex;
        uint32_t grassCount = density * density * GRASS_PATCH * GRASS_PATCH;

        uint32_t instanceCount = grassCount;
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, indexRange.first, indexRange.second, instanceCount);
    }

    glBindVertexArray(0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

    glEnable(GL_CULL_FACE);

    PopDebugGL();
}

void bee::GrassRenderer::UpdateMaterial(GrassChunkMaterial material)
{
    m_material = material;
    *m_impl->m_materialBuffer.get() = material;
    m_impl->m_materialBuffer.Patch();
}


void bee::GrassRenderer::Impl::CreateGrassBladeGeom()
{
    // Generate GL buffers.
    glGenVertexArrays(1, &m_grassBladeVAO);
    glGenBuffers(1, &m_grassBladeVBO);

    glBindVertexArray(m_grassBladeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_grassBladeVBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GrassVertex), reinterpret_cast<void*>(offsetof(GrassVertex, position)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GrassVertex), reinterpret_cast<void*>(offsetof(GrassVertex, uv0)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(GrassVertex), reinterpret_cast<void*>(offsetof(GrassVertex, uv1)));

    std::vector<GrassVertex> data{};

    const float totalHeight = 1.0f;

    uint32_t prevIndexCount{ 0 };
    for (uint32_t i = 0; i < m_LODIndex.size(); ++i)
    {
        const uint32_t indexCount = i == 2 ? 4 : 14 - i * 4;

        m_LODIndex[i].first = prevIndexCount;
        m_LODIndex[i].second = indexCount;
        for (uint32_t j = 0; j < indexCount; j += 2)
        {
            const float baseWidth = 0.025f;
            const float width = baseWidth - j * (baseWidth / (indexCount - 2)) + baseWidth * 0.1f;
            data.emplace_back(GrassVertex{
                glm::vec3{-width, 0.0f, j * (totalHeight / (indexCount - 2))},
                glm::vec2{0.0f, static_cast<float>(j) / (indexCount - 2.0f)},
                glm::vec2{0.0f, static_cast<float>(j) / (indexCount - 2.0f)},
                              });
            data.emplace_back(GrassVertex{
                glm::vec3{width, 0.0f, j * (totalHeight / (indexCount - 2))},
                glm::vec2{1.0f, static_cast<float>(j) / (indexCount - 2.0f)},
                glm::vec2{1.0f, static_cast<float>(j) / (indexCount - 2.0f)}
                              });
        }

        prevIndexCount += indexCount;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(GrassVertex) * data.size(), data.data(), GL_STATIC_DRAW);

    // Reset buffers.
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

