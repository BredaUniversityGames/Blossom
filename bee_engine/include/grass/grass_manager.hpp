#pragma once

#include <array>

#include "glm/vec3.hpp"
#include <resources/resource_handle.hpp>
#include <entt/entity/fwd.hpp>

namespace bee
{
class GrassManager
{
public:
    GrassManager();

    void Update(float dt);
    entt::entity CreateChunk(glm::vec3 position, ResourceHandle<Image> heightmap, float terrainHeight);

private:
    std::array<uint32_t, 3> m_lodRange = { 0, 64, 96 };
};
};