#pragma once

#include <tools/pcg_rand.hpp>
#include <resources/resource_handle.hpp>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>

namespace bee
{

struct prop_struct;
class Shader;

class PropBuilder
{
public:
	struct GenerationParams {

		mutable pcg::RNGState rng;
		glm::vec2 terrainSize;

		ResourceHandle<Image> densityMap;
		ResourceHandle<Image> heightMap;
		
		float matchSlopePercentage = 0.5f;
		float minScale = 0.5f;
		float maxScale = 1.5f;
		float propSpacing = 10.0f;
		float heightModifier = 1.0f;
	};


	PropBuilder();
	~PropBuilder();

	std::vector<glm::mat4> GeneratePositions(const GenerationParams& params) const;

private:

	void ComputeFilter(
		std::vector<prop_struct>& propData,
		const glm::vec2 terrainSize,
		float heightMod,
		ResourceHandle<Image> densityMap, 
		ResourceHandle<Image> heightMap
	) const;

	std::shared_ptr<bee::Shader> m_adjustPropPosition;
	uint32_t m_positionSSBO;
};

}