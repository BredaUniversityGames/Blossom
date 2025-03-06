#include <precompiled/engine_precompiled.hpp>
#include "terrain/terrain_collider.hpp"

#include <tinygltf/stb_image.h>

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/transform.hpp"
#include "terrain/terrain_chunk.hpp"
#include "tools/log.hpp"

bee::TerrainCollider::TerrainCollider(FileIO::Directory directory, const std::string& heightMapPath) : m_heightMapTransform(glm::identity<glm::mat4>())
{
    std::vector<char> heightMapBinary = Engine.FileIO().ReadBinaryFile(directory, heightMapPath);
    int component{};
    m_heightMap = stbi_load_from_memory(
        reinterpret_cast<unsigned char*>(heightMapBinary.data()),
        static_cast<int>(heightMapBinary.size()), &m_heightMapWidth, &m_heightMapHeight, &component, 4
    );
}

bee::TerrainCollider::~TerrainCollider()
{
    stbi_image_free(m_heightMap);
}

float bee::TerrainCollider::SampleHeightInWorld(glm::vec3 worldPosition) const
{
    auto view = Engine.ECS().Registry.view<TerrainChunk, Transform>();
    auto [terrain, transform] = view.get(*view.begin());

    glm::vec4 sampleLocation = GetHeightMapTransform() * glm::vec4(worldPosition, 1.0f);

    if (sampleLocation.x - 1.0f < 0 || 
        sampleLocation.y - 1.0f < 0 || 
        sampleLocation.x + 1.0f > m_heightMapWidth || 
        sampleLocation.y + 1.0f > m_heightMapHeight)
        return 0.0f;

    uint32_t x0{ static_cast<uint32_t>(sampleLocation.x) };
    uint32_t y0{ static_cast<uint32_t>(sampleLocation.y) };
    uint32_t x1{ x0 + 1 };
    uint32_t y1{ y0 + 1 };

    float dx = sampleLocation.x - x0;
    float dy = sampleLocation.y - y0;

    uint8_t topLeft     = m_heightMap[(y0 * m_heightMapWidth + x0) * 4];
    uint8_t topRight    = m_heightMap[(y0 * m_heightMapWidth + x1) * 4];
    uint8_t bottomLeft  = m_heightMap[(y1 * m_heightMapWidth + x0) * 4];
    uint8_t bottomRight = m_heightMap[(y1 * m_heightMapWidth + x1) * 4];

    float top    = topLeft    + dx * static_cast<float>(topRight    - topLeft);
    float bottom = bottomLeft + dx * static_cast<float>(bottomRight - bottomLeft);

    float value = top + dy * (bottom - top);

    float height = value / 255.0f * terrain.heightModifier;

    return height;
}

glm::mat4 bee::TerrainCollider::GetHeightMapTransform() const
{
    if (m_initialized)
        return m_heightMapTransform;

    auto view = Engine.ECS().Registry.view<TerrainChunk, Transform>();
    if (view.begin() == view.end())
    {
        return glm::identity<glm::mat4>();
    }

    auto [terrain, transform] = view.get(*view.begin());
    m_heightMapTransform *= glm::scale(glm::identity<glm::mat4>(), glm::vec3{ m_heightMapWidth, m_heightMapHeight, 1.0f });
    m_heightMapTransform *= glm::translate(glm::identity<glm::mat4>(), glm::vec3{ 0.5f, 0.5f, 0.0f }); // Translate by half to center
    m_heightMapTransform *= glm::scale(glm::identity<glm::mat4>(), 1.0f / glm::vec3{ terrain.width, terrain.height, 1.0f }); // Scale by terrain size.
    m_heightMapTransform *= glm::inverse(transform.World());

    m_initialized = true;

    return m_heightMapTransform;
}
