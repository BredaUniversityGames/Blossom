#pragma once

#include <memory>
#include "resources/resource_handle.hpp"

namespace bee
{

struct TerrainChunk
{
    TerrainChunk() = default;

    int width   = 0;
    int height  = 0;
    ResourceHandle<Image> heightmap = {};
    float tesselationFactor     = 1;
    float tesselationDistance   = 0;
    float heightModifier        = 8;
    float normalScale           = 1;
};

}
