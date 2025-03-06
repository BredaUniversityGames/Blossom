#pragma once
#include <code_utils/bee_utils.hpp>
#include <memory>
#include <level_editor/edit_state.hpp>
#include <level_editor/prop_builder.hpp>
#include <menu_interface.hpp>

namespace bee {

class Level;

class LevelEditor : public IMenu {
public:

	LevelEditor();
	~LevelEditor() = default;

	virtual void Show(BlossomGame& game) override;

	//Called when the current level changes
	virtual void LoadFromFile(BlossomGame& game, cereal::JSONInputArchive* input) override;

	//Called when the current level is saved
	virtual void SaveToFile(BlossomGame& game, cereal::JSONOutputArchive& output) const override;

	//Builds and generates scene into games currentLevel.
	//Anything unchanged is not regenerated
	virtual void ApplyChanges(BlossomGame& game) override;

	NON_COPYABLE(LevelEditor);
	NON_MOVABLE(LevelEditor);

private:

	//UI
	void GlobalSettingsMenu();
	void LightingSettingsMenu();
	void TerrainSettingsMenu();
	void GrassSettingsMenu();

	void PropGlobalSettingsMenu();
	void PropSettingsMenu(EditState::PropEditState& prop);

	//Baking
	bool BakeTerrain(std::shared_ptr<Level> level) const;
	bool BakeGrass(std::shared_ptr<Level> level) const;
	bool BakeProp(std::shared_ptr<Level> level, size_t prop_index) const;

	EditState m_editState{};
	PropBuilder m_propBuilder{};

	//Every frame the editor checks if its referencing the same level
	//If not, it will switch to the new one
	std::shared_ptr<Level> m_currentLevel;
};

}