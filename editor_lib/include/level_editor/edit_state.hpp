#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

namespace bee {

/// <summary>
/// EditState represent the parameters that are saved and loaded in editor
/// to generate the level. Many fields are similar to level while including more parameters for generation
/// </summary>
struct EditState {

	struct TerrainEditState
	{
		std::filesystem::path terrainHeighMapPath;

		std::filesystem::path terrainAlbedoPath;
		std::filesystem::path terrainNormalPath;
		std::filesystem::path terrainRoughnessPath;

		int mapSizeX = 256; // TODO: doesnt work with non square sizes
		int mapSizeY = 256;
		int unitsPerTile = 1; //TODO: currently doesnt work with grass

		float tesselationDistance = 50.0f;
		float tesselationModifier = 8.0f;
		float heightScale = 2.0f;
	};

	struct GrassEditState
	{
		std::filesystem::path grassDensityPath;
		std::filesystem::path grassColourPath;
	};

	struct PropEditState
	{
		int propSeed = 1;
		float propSpacing = 10.0f;

		std::vector<std::filesystem::path> propModelPaths;
		std::filesystem::path propDensityMap;

		float adjustSlopePercentage = 0.2f;

		float displacementHeightPercent = 0.5f;
		float distanceMaxBend = 2.0f;
		bool generateWindMask = true;
		bool generateCollidableMesh = false;
		
		bool collectable = false;
		bool partOfSequence = false;

		bool propDirty = false;
	};

	std::string currentLevelPath;
	
	TerrainEditState terrainEdit;
	bool terrainDirty = false;

	GrassEditState grassEdit;
	bool grassDirty = false;

	std::vector<PropEditState> propEdits;
};

template<typename A>
void save(A& archive, const EditState::PropEditState& desc)
{
	archive(cereal::make_nvp("Seed", desc.propSeed));

	std::vector<std::string> pathStrings;
	pathStrings.reserve(desc.propModelPaths.size());
	for (auto& path : desc.propModelPaths)
	{
		pathStrings.push_back(path.string());
	}

	archive(cereal::make_nvp("ModelPaths", pathStrings));

	archive(cereal::make_nvp("DensityMap", desc.propDensityMap.string()));

	archive(cereal::make_nvp("Spacing", desc.propSpacing));
	archive(cereal::make_nvp("UseWind", desc.generateWindMask));
	archive(cereal::make_nvp("GenerateCollidableMesh", desc.generateCollidableMesh));
	archive(cereal::make_nvp("Collectable", desc.collectable));
	archive(cereal::make_nvp("HiddenUntilSequence", desc.partOfSequence));
	archive(cereal::make_nvp("AdjustSlopePercentage", desc.adjustSlopePercentage));
	archive(cereal::make_nvp("MaxBendFactor", desc.distanceMaxBend));
	archive(cereal::make_nvp("BendHeightPercentage", desc.displacementHeightPercent));
}

template<typename A>
void load(A& archive, EditState::PropEditState& desc)
{
	archive(cereal::make_nvp("Seed", desc.propSeed));

	std::vector<std::string> modelPaths;
	archive(cereal::make_nvp("ModelPaths", modelPaths));
	
	std::string density;
	archive(cereal::make_nvp("DensityMap", density));

	desc.propModelPaths.clear();
	for (auto& path : modelPaths)
	{
		desc.propModelPaths.push_back(path);
	}

	desc.propDensityMap = density;

	archive(cereal::make_nvp("Spacing", desc.propSpacing));
	archive(cereal::make_nvp("UseWind", desc.generateWindMask));
	archive(cereal::make_nvp("GenerateCollidableMesh", desc.generateCollidableMesh));
	archive(cereal::make_nvp("Collectable", desc.collectable));
	archive(cereal::make_nvp("HiddenUntilSequence", desc.partOfSequence));
	archive(cereal::make_nvp("MaxBendFactor", desc.distanceMaxBend));
	archive(cereal::make_nvp("AdjustSlopePercentage", desc.adjustSlopePercentage));
	archive(cereal::make_nvp("BendHeightPercentage", desc.displacementHeightPercent));
}

}