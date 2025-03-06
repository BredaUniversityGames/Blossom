#pragma once
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <visit_struct/visit_struct.hpp>

#include "math/geometry.hpp"
#include "resources/resource_handle.hpp"

namespace bee
{
constexpr uint32_t GRASS_PATCH = 16;
class Image;

struct CulledGrass{};

struct GrassChunk
{
    uint32_t columns{ 1 }, rows{ 1 };
    uint32_t lod{ 0 };
    ResourceHandle<bee::Image> heightMapImage{ nullptr };
    BoundingBox bounds{};

    GrassChunk() = default;

    GrassChunk(uint32_t columns, uint32_t rows, uint32_t lod, ResourceHandle<bee::Image> heightMapImage)
        : columns(columns), rows(rows), lod(lod), heightMapImage(heightMapImage)
    {
        bounds = BoundingBox{glm::vec3{0.0f}, glm::vec3{columns / 2.0f, rows / 2.0f, 1.0f}};
    }

    uint32_t GrassCount() const { return columns * rows * GRASS_PATCH * GRASS_PATCH; }

};

struct GrassChunkMaterial
{
    glm::vec3 color{ 71.0f / 255.0f, 110.0f / 255.0f, 47.0f / 255.0f };
    float minHeight{ 0.9f };
    float maxHeight{ 1.4f };
    float cutoffLength{ 0.3f };
    float bladeBending{ 0.5f };
    float distanceNormalBlend{ 10.0f };
    float darkEdgeStrength{ 0.5f };
    float darkEdgeRange{ 0.0f };
    float aoStrength{ 0.5f };
    float aoRange{ 0.3f };
    float SSSstrength{ 1.0f };
    float SSSdistortion{ 0.75f };
    float SSSpower{ 0.85f };
    float _padding;
    std::array<float, 4> lodAdjustments{ 1.0f, 1.0f, 1.0f, 1.0f };
};

template<typename A>
void serialize(A& archive, bee::GrassChunkMaterial& desc)
{
    archive(cereal::make_nvp("MinHeight", desc.minHeight));
    archive(cereal::make_nvp("MaxHeight", desc.maxHeight));
    archive(cereal::make_nvp("CutoffLength", desc.cutoffLength));
    archive(cereal::make_nvp("BladeBending", desc.bladeBending));
    archive(cereal::make_nvp("DarkEdgeStrength", desc.darkEdgeStrength));
    archive(cereal::make_nvp("DarkEdgeRange", desc.darkEdgeRange));
    archive(cereal::make_nvp("AOStrength", desc.aoStrength));
    archive(cereal::make_nvp("AORange", desc.aoRange));
    archive(cereal::make_nvp("SSSStrength", desc.SSSstrength));
    archive(cereal::make_nvp("SSSDistorion", desc.SSSdistortion));
    archive(cereal::make_nvp("SSSPower", desc.SSSpower));
    archive(cereal::make_nvp("LODAdjustments", desc.lodAdjustments));
}
}

VISITABLE_STRUCT(bee::GrassChunk, columns, rows, lod);
