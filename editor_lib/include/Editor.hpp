#pragma once
#include <code_utils/bee_utils.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <string>

//Declaration of main Editor class

namespace bee {

class IMenu;
class BlossomGame;
class Level;

class Editor {
public:

	Editor();
	~Editor();

	NON_COPYABLE(Editor);
	NON_MOVABLE(Editor);

	void UpdateSystems(BlossomGame& game, float dt);

	//Triggered when Build level is pressed
	void ApplyChanges(BlossomGame& game);

	//Triggered when Saving a level to disk
	void LoadMenuData(BlossomGame& game);

	//Triggered after a level is successfully loaded in the engine
	void SaveMenuData(BlossomGame& game);

private:

	//If the level is not bound to a filepath, this will open the file dialog
	//And force the user to pick a path
	//return nullopt if the user does not select a path
	std::optional<std::string> CheckLevelPath(std::shared_ptr<Level> level);

	//Stores names and menu ptrs
	//Menu name, active, menu ptr
	std::unordered_map<std::string, std::pair<bool, std::unique_ptr<IMenu>>> m_menus;
};

}