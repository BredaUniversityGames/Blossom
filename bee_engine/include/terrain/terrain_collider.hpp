#pragma once
#include <code_utils/bee_utils.hpp>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/fwd.hpp>

#include "core/fileio.hpp"

namespace bee
{
class TerrainCollider
{
public:
    TerrainCollider(bee::FileIO::Directory directory, const std::string& heightMapPath);
    ~TerrainCollider();

    float SampleHeightInWorld(glm::vec3 worldPosition) const;

private:
    NON_COPYABLE(TerrainCollider);
    NON_MOVABLE(TerrainCollider);

    uint8_t* m_heightMap;
    int32_t m_heightMapWidth;
    int32_t m_heightMapHeight;

    mutable glm::mat4 m_heightMapTransform;
    mutable bool m_initialized{ false };

    glm::mat4 GetHeightMapTransform() const;
};
}

