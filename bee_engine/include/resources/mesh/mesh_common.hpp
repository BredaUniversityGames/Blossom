#pragma once

#include "resources/resource_handle.hpp"
#include <glm/glm.hpp>

namespace bee
{

namespace mesh_utils
{
	void GenerateDisplacementData(ResourceHandle<Mesh> mesh, glm::mat4 modelMatrix, glm::vec3 minBounds, glm::vec3 maxBounds, float trunkPercentage, float distanceMaxBend);
}

}