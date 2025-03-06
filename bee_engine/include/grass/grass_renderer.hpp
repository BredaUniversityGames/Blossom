#pragma once

#include <memory>
#include <entt/entity/fwd.hpp>

#include "resources/resource_handle.hpp"
#include <glm/glm.hpp>

#include "resources/material/material.hpp"
#include "grass_chunk.hpp"

#if defined(BEE_PLATFORM_PC)
#include <resources/image/image_gl.hpp>
#endif

namespace bee
{

class Shader;

struct GrassVertex
{
    glm::vec3 position;
    glm::vec2 uv0;
    glm::vec2 uv1;
};

class GrassRenderer
{
public:
    GrassRenderer(const Material::IBL& ibl);
    ~GrassRenderer();

    void OnGrassChunkCreate(entt::registry& registry, entt::entity entity);
    void OnGrassChunkDestroy(entt::registry& registry, entt::entity entity);

    void Render();

    struct GrassMaps
    {
        ResourceHandle<Image> m_colorMap;
        ResourceHandle<Image> m_lengthMap;
    };

    GrassMaps& GetInputMaps() { return m_maps; }
    GrassChunkMaterial GetMaterial() const { return m_material; }
    void UpdateMaterial(GrassChunkMaterial material);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    ResourceHandle<Image> m_noiseImage;
    ResourceHandle<Image> m_windNoiseImage;
    GrassMaps m_maps;
    std::shared_ptr<Shader> m_grassCompute;
    std::shared_ptr<Shader> m_grassPass;
    const Material::IBL& m_ibl;
    GrassChunkMaterial m_material{};
};
};