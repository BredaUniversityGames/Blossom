#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <resources/resource_handle.hpp>
#include <core/fileio.hpp>
#include <tools/serialization.hpp>
#include <vector>

#include "grass/grass_chunk.hpp"

namespace bee {

class TerrainCollider;

class Level {
public:

	struct LightingDescription {

		std::string skyboxPath;
		glm::quat sunDirection{ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 sunColour{ 1.0f };
		float sunIntensity = 100.0f;

		float ambientFactor{ 0.2f };
		float SSSstrength{ 1.0f };
		float SSSdistortion{ 0.75f };
		float SSSpower{ 0.85f };
	};

	//TODO: not used
	struct CameraDescription {

		glm::vec3 initialPosition;
		//TODO: more camera params

	};

	//TODO not used or serialized or finished even
	struct PostProcessingDescription {

		//Vignette
		bool enableVignette = false;
		float vignetteBorderFactor = 0.0f; //from 0 to 1

		//Bloom options
		uint32_t blurPasses = 2;

		//TODO: more parameters and tonemapping settings
	};

	struct TerrainDescription
	{
		ResourceHandle<Image> heightMap;
		ResourceHandle<Material> terrainMaterial;
		ResourceHandle<Mesh> generatedMesh;

		int mapSizeX = 256; // TODO: doesnt work with non square sizes
		int mapSizeY = 256;
		int unitsPerTile = 1; //TODO: currently doesnt work with grass

		float tesselationDistance = 50.0f;
		float tesselationModifier = 8.0f;
		float heightScale = 2.0f;
	};

	struct GrassDescription
	{
		ResourceHandle<Image> grassColour;
		ResourceHandle<Image> grassDensityMap;
		GrassChunkMaterial material;
	};

	struct PropDescription
	{
		std::vector<ResourceHandle<Model>> propModels;
		std::vector<glm::mat4> propTransforms;

		//Wind generation
		float displacementHeightPercent = 0.5f;
		float distanceMaxBend = 2.0f;
		bool generateWindMask = true;
		bool generateCollidableMesh = false;
		bool collectable = false;
		bool partOfSequence = false;
	};

	struct LODDescription
    {
        std::array<float, 2> distances{};
    };

	// Generates the default level
	Level();

	// Generates the level from JSON Archive
	Level(JSONLoader& archive);

	~Level();

	NON_COPYABLE(Level);

	const TerrainCollider& GetTerrainCollider() const { return *m_terrainCollider; }

	auto& GetLighting() { return m_lighting; }
	auto& GetTerrain() { return m_terrain; }
	auto& GetGrass() { return m_grass; }
	auto& GetPropList() { return m_props; }
	auto& GetLODs() { return m_lods; }

	void GenerateAll() {
		GenerateTerrain(); GenerateGrass(); GenerateLighting(); GenerateAllProps();
	}

	void GenerateTerrain();
	void GenerateGrass();

	//Updates skybox
	void GenerateLighting();

	void GenerateAllProps() {
		for (size_t i = 0; i < m_props.size(); i++) GenerateProp(i);
	}

	struct ModelTag { size_t index; };
	void GenerateProp(size_t prop_index);
	void ClearProp(size_t prop_index);

	//Serialization
	void SaveToArchive(JSONSaver& archive);

	void FallbackDefaultLevel();

	std::string GetPath() const { return m_originPath; }
	void SetPath(const std::string& path) { m_originPath = path; }

private:

	std::string m_originPath{};

	LightingDescription m_lighting;
	TerrainDescription m_terrain;
	GrassDescription m_grass;
	LODDescription m_lods;

	std::vector<PropDescription> m_props;

	std::unique_ptr<TerrainCollider> m_terrainCollider;

};

}
