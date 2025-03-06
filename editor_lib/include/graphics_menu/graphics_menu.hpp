#pragma once
#include <code_utils/bee_utils.hpp>
#include <menu_interface.hpp>
#include <filesystem>

namespace bee {

class Level;

class GraphicsMenu : public IMenu {
public:
	GraphicsMenu() = default;
	~GraphicsMenu() = default;

	virtual void Show(BlossomGame& game) override;
	virtual void LoadFromFile(BlossomGame& game, cereal::JSONInputArchive* in_archive);
	virtual void ApplyChanges(BlossomGame& game) override;

	NON_COPYABLE(GraphicsMenu);
	NON_MOVABLE(GraphicsMenu);

private:

	void DrawLightingMenu(std::shared_ptr<Level> level);
	void DrawPostProcessingMenu(std::shared_ptr<Level> level);
	void DrawGrassMenu(std::shared_ptr<Level> level);
	void DrawLODsMenu(std::shared_ptr<Level> level);
    void DrawDebugFlags();
	void DrawSubsurfaceMenu(std::shared_ptr<Level> level);

	bool skyboxDirty = false;
	std::filesystem::path skyboxEdit;
};

}