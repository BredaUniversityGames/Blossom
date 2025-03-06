#include <precompiled/editor_precompiled.hpp>
#include <level_editor/level_editor.hpp>
#include <core/engine.hpp>
#include <tools/log.hpp>
#include <level/level.hpp>
#include <asset_browser/asset_types.hpp>
#include <resources/image/image_loader.hpp>
#include <resources/resource_manager.hpp>
#include <cereal/archives/json.hpp>

namespace bee {

bool DisplayTextureField(std::string name, std::filesystem::path& path) {

	std::string displayName = "No " + name;
	if (!path.empty()) {
		displayName = name + ": " + path.string();
	}

	ImGui::Text(displayName.c_str());
	if (AssetDropTarget<Image>(&path)) {
		return true;
	}
	return false;
}

}


bee::LevelEditor::LevelEditor()
{

}

void bee::LevelEditor::Show(BlossomGame& game)
{
	ImGui::Begin("Level Editor");

	if (game.GetLevel() == nullptr) {
		ImGui::Text("No Level loaded");
		ImGui::End();

		return;
	}

	ImGui::Text("Edit Current Level");

	ImGui::Separator();
	TerrainSettingsMenu();

	ImGui::Separator();
	GrassSettingsMenu();

	ImGui::Separator();
	PropGlobalSettingsMenu();

	ImGui::End();
}

void bee::LevelEditor::LoadFromFile(BlossomGame& game, cereal::JSONInputArchive* input)
{
	m_editState = EditState{};
	auto level = game.GetLevel();

	//Load Terrain edits
	{
		auto& terrainData = level->GetTerrain();
		m_editState.terrainEdit.heightScale = terrainData.heightScale;
		m_editState.terrainEdit.mapSizeX = terrainData.mapSizeX;
		m_editState.terrainEdit.mapSizeY = terrainData.mapSizeY;

		m_editState.terrainEdit.terrainHeighMapPath = terrainData.heightMap.GetPath();

		if (auto mat = terrainData.terrainMaterial.Retrieve()) {
			m_editState.terrainEdit.terrainAlbedoPath = mat->BaseColorTexture.GetPath();
			m_editState.terrainEdit.terrainNormalPath = mat->NormalTexture.GetPath();
			m_editState.terrainEdit.terrainRoughnessPath = mat->MetallicRoughnessTexture.GetPath();
		}
	}

	//Load Grass Edits
	{
		auto& grassData = level->GetGrass();
		m_editState.grassEdit.grassColourPath = grassData.grassColour.GetPath();
		m_editState.grassEdit.grassDensityPath = grassData.grassDensityMap.GetPath();
	}

	//Load Prop edits
	{
		for (auto& prop : level->GetPropList()) {
			m_editState.propEdits.emplace_back();
			auto& editProp = m_editState.propEdits.back();
			editProp.propModelPaths.clear();
			for (int i = 0; i < prop.propModels.size(); i += 1)
			{
				editProp.propModelPaths.push_back(prop.propModels[i].GetPath());
			}
		}
	}

	m_editState.currentLevelPath = level->GetPath();

	if (input) {
		(*input)(cereal::make_nvp("PropEdits", m_editState.propEdits));
	}
	else {
		Log::Warn("No .editor file loaded for the level");
	}

	m_editState.terrainDirty = false;
	m_editState.grassDirty = false;
	for (auto& prop : m_editState.propEdits) prop.propDirty = false;
}

void bee::LevelEditor::SaveToFile(BlossomGame& game, cereal::JSONOutputArchive& output) const
{
	//Only saving prop edits, since they are the only ones that are not baked
	output(cereal::make_nvp("PropEdits", m_editState.propEdits));
}

void bee::LevelEditor::ApplyChanges(BlossomGame& game)
{
	if (auto level = game.GetLevel()) {

		if (m_editState.terrainDirty) {

			//If the terrain changes, everything else is wrongly placed
			m_editState.grassDirty = true;
			for (auto& prop : m_editState.propEdits) prop.propDirty = true;

			auto success = BakeTerrain(level);
			if (!success) return;

			m_editState.terrainDirty = false;
		}


		if (m_editState.grassDirty) {

			BakeGrass(level);
			m_editState.grassDirty = false;
		}

		int editPropCount = static_cast<int>(m_editState.propEdits.size());
		int instantiatedPropCount = static_cast<int>(level->GetPropList().size());

		//If there are less instantiated elements in the level prop list, add more empty prop fields to generate
		if (auto diff = editPropCount - instantiatedPropCount; diff > 0) {
			for (int i = 0; i < diff; i++) level->GetPropList().emplace_back();
		} 
		//Otherwise, clear all props if they have been removed from the edit list
		else if (auto diff = editPropCount - instantiatedPropCount; diff < 0) {
			for (int i = 0; i < -diff; i++) {
				level->ClearProp(level->GetPropList().size() - 1);
				level->GetPropList().pop_back();
			}
		}

		for (size_t i = 0; i < m_editState.propEdits.size(); i++) {
			
			if (m_editState.propEdits.at(i).propDirty) {
				BakeProp(level, i);
				m_editState.propEdits.at(i).propDirty = false;
			}
		}

	}
	else {
		Log::Warn("Tried to Bake a Level when there is no Level currently loaded");
	}
}

void bee::LevelEditor::GlobalSettingsMenu()
{
}

void bee::LevelEditor::LightingSettingsMenu()
{
	//TODO: No lighting settings implemented
}

void bee::LevelEditor::TerrainSettingsMenu()
{
	auto& terrainSettings = m_editState.terrainEdit;
	bool edited = false;

	ImGui::Text("Edit Terrain");

	//Edit size

	int new_size[2] = { terrainSettings.mapSizeX, terrainSettings.mapSizeY };
	if (ImGui::DragInt2("Terrain Size", new_size)) {

		terrainSettings.mapSizeX = new_size[0];
		terrainSettings.mapSizeY = new_size[1];

		edited = true;
	}

	//Edit scale
	if (ImGui::DragFloat("Terrain Height Scale", &terrainSettings.heightScale))
		edited = true;

	//Edit heightmap

	if (DisplayTextureField("Heightmap", terrainSettings.terrainHeighMapPath))
		edited = true;

	if (DisplayTextureField("Terrain Albedo", terrainSettings.terrainAlbedoPath))
		edited = true;

	if (DisplayTextureField("Terrain Normal", terrainSettings.terrainNormalPath))
		edited = true;

	if (DisplayTextureField("Terrain Roughness", terrainSettings.terrainRoughnessPath))
		edited = true;

	if (edited) m_editState.terrainDirty = true;
}

void bee::LevelEditor::GrassSettingsMenu()
{
	//TODO: Currently no edit fields for grass are implemented
	ImGui::Text("Edit Grass");

	bool edited = false;
	auto& grassSettings = m_editState.grassEdit;

	if (DisplayTextureField("Colour Map", grassSettings.grassColourPath))
		edited = true;

	if (DisplayTextureField("Density Map", grassSettings.grassDensityPath))
		edited = true;

	if (edited) m_editState.grassDirty = true;
}

void bee::LevelEditor::PropGlobalSettingsMenu()
{
	ImGui::Text("Edit Props");

	ImGui::SameLine();
	ImGui::Dummy({ 15, 0 });
	ImGui::SameLine();

	auto& propEditState = m_editState.propEdits;

	if (ImGui::Button("Add")) {
		propEditState.emplace_back();
	}

	for (size_t i = 0; i < propEditState.size();)
	{
		ImGui::PushID(i);

		bool remove = ImGui::Button(" - ");
		ImGui::SameLine();

		auto& prop = propEditState[i];
		PropSettingsMenu(prop);

		ImGui::PopID();

		if (remove) { 
			propEditState.erase(propEditState.begin() + i); 
			for (auto& prop : propEditState) prop.propDirty = true;
		}
		else { i++; }
	}

}

void bee::LevelEditor::PropSettingsMenu(EditState::PropEditState& prop)
{
	std::string propName = "Empty Prop";
	if (!prop.propModelPaths.empty() && !prop.propModelPaths[0].stem().string().empty())
	{
		propName = prop.propModelPaths[0].stem().string();
	}

	bool edited = false;

	if (ImGui::TreeNode(propName.c_str())) {
		
		//Seed

		if (ImGui::DragInt("Input Seed", &prop.propSeed)) {
			edited = true;
		}

		//Model

		for (int i = 0; i < prop.propModelPaths.size(); i += 1)
		{
			ImGui::PushID(i);

			bool remove = ImGui::Button(" - ");

			std::string displayName = "No Model bound";
			if (!prop.propModelPaths[i].empty()) {
				displayName = "Model " + std::to_string(i) + ": " + prop.propModelPaths[i].stem().string();
			}

			ImGui::SameLine();
			ImGui::Text(displayName.c_str());
			if (AssetDropTarget<Model>(&prop.propModelPaths[i])) {
				edited = true;
			}

			if (remove)
			{
				prop.propModelPaths.erase(prop.propModelPaths.begin() + i);
				edited = true;
			}

			ImGui::PopID();
		}

		if (ImGui::Button("+ Add Model"))
		{
			prop.propModelPaths.push_back("");
		}

		//Density map

		std::string displayName = "No Density Map Bound";
		if (!prop.propDensityMap.empty()) {
			displayName = "Density Map: " + prop.propDensityMap.stem().string();
		}

		ImGui::Text(displayName.c_str());
		if (AssetDropTarget<Image>(&prop.propDensityMap)) {
			edited = true;
		}

		//Cell Spacing
		if (ImGui::DragFloat("Cell Spacing", &prop.propSpacing, 0.1f)) {
			edited = true;
		}

		//Slope
		if (ImGui::SliderFloat("Adjust Slope Factor", &prop.adjustSlopePercentage, 0.0f, 1.0f)) {
			edited = true;
		}

		if (ImGui::Checkbox("Is Collectable", &prop.collectable))
		{
			edited = true;
		}

		if (ImGui::Checkbox("Hidden until Beautify Sequence", &prop.partOfSequence))
		{
			edited = true;
		}

		if (ImGui::Checkbox("Generate wind mask", &prop.generateWindMask))
		{
			edited = true;
		}

		if (ImGui::Checkbox("Generate collidable mesh", &prop.generateCollidableMesh))
		{
			edited = true;
		}

		if (prop.generateWindMask)
		{
			if (ImGui::DragFloat("Trunk height percentage", &prop.displacementHeightPercent, 0.1f, 0.0f, 1.0f))
			{
				edited = true;
			}

			if (ImGui::DragFloat("Max bend distance", &prop.distanceMaxBend))
			{
				edited = true;
			}
		}

		ImGui::TreePop();
	}

	if (edited) prop.propDirty = true;
}

bool bee::LevelEditor::BakeTerrain(std::shared_ptr<Level> level) const
{
	auto& terrainData = level->GetTerrain();
	terrainData.mapSizeX = m_editState.terrainEdit.mapSizeX;
	terrainData.mapSizeY = m_editState.terrainEdit.mapSizeY;
	terrainData.heightScale = m_editState.terrainEdit.heightScale;
	
	auto& image_loader = Engine.Resources().Images();

	try {
		terrainData.heightMap = image_loader
			.FromFile(bee::FileIO::Directory::Asset, m_editState.terrainEdit.terrainHeighMapPath.string(), ImageFormat::RGBA8);
	}
	catch (std::exception& e) {
		Log::Error("Generating Terrain: {}", e.what());
		return false;
	}

	//Attempt to load material

	auto sampler = bee::Sampler();
	sampler.MagFilter = Sampler::Filter::Linear;
	sampler.MinFilter = Sampler::Filter::LinearMipmapLinear;
	sampler.WrapS = Sampler::Wrap::Repeat;
	sampler.WrapT = Sampler::Wrap::Repeat;

	auto matBuilder = MaterialBuilder();
	auto& imageLoader = Engine.Resources().Images();

	if (!m_editState.terrainEdit.terrainAlbedoPath.empty())
		matBuilder.WithTexture(TextureSlotIndex::BASE_COLOR,
			imageLoader.FromFile(FileIO::Directory::Asset, m_editState.terrainEdit.terrainAlbedoPath.string(), ImageFormat::RGBA8));

	if (!m_editState.terrainEdit.terrainNormalPath.empty())
		matBuilder.WithTexture(TextureSlotIndex::NORMAL_MAP, 
			imageLoader.FromFile(FileIO::Directory::Asset, m_editState.terrainEdit.terrainNormalPath.string(), ImageFormat::RGBA8));

	if (!m_editState.terrainEdit.terrainRoughnessPath.empty())
		matBuilder.WithTexture(TextureSlotIndex::METALLIC_ROUGHNESS,
			imageLoader.FromFile(FileIO::Directory::Asset, m_editState.terrainEdit.terrainRoughnessPath.string(), ImageFormat::RGBA8));

	terrainData.terrainMaterial = matBuilder
		.WithSampler(TextureSlotIndex::BASE_COLOR, sampler)
		.WithSampler(TextureSlotIndex::NORMAL_MAP, sampler)
		.WithSampler(TextureSlotIndex::METALLIC_ROUGHNESS, sampler)
		.Build();

	level->GenerateTerrain();
	return true;
}

bool bee::LevelEditor::BakeGrass(std::shared_ptr<Level> level) const
{
	auto& grassData = level->GetGrass();
	auto& imageLoader = Engine.Resources().Images();

	try {
		grassData.grassColour = imageLoader
			.FromFile(bee::FileIO::Directory::Asset, m_editState.grassEdit.grassColourPath.string(), ImageFormat::RGBA8);
	}
	catch (std::exception& e) {
		Log::Error("Generating Grass: {}", e.what());
		return false;
	}

	try {
		grassData.grassDensityMap = imageLoader
			.FromFile(bee::FileIO::Directory::Asset, m_editState.grassEdit.grassDensityPath.string(), ImageFormat::RGBA8);
	}
	catch (std::exception& e) {
		Log::Error("Generating Grass: {}", e.what());
		return false;
	}

	level->GenerateGrass();
	return true;
}

bool bee::LevelEditor::BakeProp(std::shared_ptr<Level> level, size_t prop_index) const
{
	auto& levelPropDescription = level->GetPropList().at(prop_index);
	const auto& editorPropDescription = m_editState.propEdits.at(prop_index);

	levelPropDescription.displacementHeightPercent = editorPropDescription.displacementHeightPercent;
	levelPropDescription.distanceMaxBend = editorPropDescription.distanceMaxBend;
	levelPropDescription.generateWindMask = editorPropDescription.generateWindMask;
	levelPropDescription.generateCollidableMesh = editorPropDescription.generateCollidableMesh;
	levelPropDescription.collectable = editorPropDescription.collectable;
	levelPropDescription.partOfSequence = editorPropDescription.partOfSequence;

	auto& image_loader = Engine.Resources().Images();
	try {

		auto& levelInfo = level->GetTerrain();
		glm::vec2 mapSize = { levelInfo.mapSizeX * levelInfo.unitsPerTile,  levelInfo.mapSizeY * levelInfo.unitsPerTile };

		PropBuilder::GenerationParams params;
		params.densityMap = image_loader.FromFile(FileIO::Directory::Asset, editorPropDescription.propDensityMap.string(), ImageFormat::RGBA8);
		params.heightMap = levelInfo.heightMap;
		params.heightModifier = levelInfo.heightScale;
		params.matchSlopePercentage = editorPropDescription.adjustSlopePercentage; //TODO, not serialized in prop
		params.maxScale = 1.5f;
		params.minScale = 0.5f;
		params.terrainSize = mapSize;
		params.propSpacing = editorPropDescription.propSpacing;

		pcg::RNGState PCGstate;
		PCGstate.state = (static_cast<int64_t>(editorPropDescription.propSeed) << 17) ^ 0x0ddba11deadface;
		PCGstate.inc = static_cast<int64_t>(editorPropDescription.propSeed) ^ 0x0ddba11deadface;

		params.rng = PCGstate;
		levelPropDescription.propTransforms = m_propBuilder.GeneratePositions(params);
	}
	catch (std::exception& e) {
		Log::Error("Generating Prop ID{} : {}", prop_index, e.what());
		return false;
	}

	//Load model
	auto& model_loader = Engine.Resources().Models();
	try {
		levelPropDescription.propModels.clear();
		for (int i = 0; i < editorPropDescription.propModelPaths.size(); i += 1)
		{
			ResourceHandle<Model> modelHandle = model_loader.FromGLTF(bee::FileIO::Directory::Asset, editorPropDescription.propModelPaths[i].string());
			levelPropDescription.propModels.push_back(modelHandle);
		}
	}
	catch (std::exception& e) {
		Log::Error("Generating Prop ID{} : {}", prop_index, e.what());
		return false;
	}

	level->GenerateProp(prop_index);
	return true;
}
