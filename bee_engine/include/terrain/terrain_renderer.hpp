#pragma once

#include <entt/entity/fwd.hpp>
#include <memory>

#include "resources/resource_handle.hpp"
#include "resources/material/material.hpp"

namespace bee
{
    struct DebugData;


    class TerrainRenderer
{
public:
    TerrainRenderer(const DebugData& debugFlags, const Material::IBL& ibl, uint32_t iblSpecularMipCount);
    ~TerrainRenderer();
    void Submit(entt::entity entity);
    void CleanUp(entt::entity entity);
    void Render();
    void DepthOnlyRender(std::shared_ptr<Shader> depthOnlyShader);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    ResourceHandle<Image> m_heighmapImage;
    std::shared_ptr<Shader> m_terrainPass;
    const DebugData& m_debugFlags;
    const Material::IBL& m_ibl;
    uint32_t m_iblSpecularMipCount;
};
};  // namespace bee