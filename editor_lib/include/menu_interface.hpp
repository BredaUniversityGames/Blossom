#pragma once
#include <game/blossom.hpp>

namespace cereal {
class JSONOutputArchive;
class JSONInputArchive;
}

namespace bee {


class IMenu {
public:
	virtual ~IMenu() = default;
	
	//OPTIONAL, Saving, loading and baking
	virtual void ApplyChanges(BlossomGame& game) {}

	//Saving to .editor.json file
	virtual void SaveToFile(BlossomGame& game, cereal::JSONOutputArchive& out_archive) const {}

	//Loading from .editor.json file (pass nullptr if file does not exist)
	//Passing nullptr is possible since some systems also load from level data
	virtual void LoadFromFile(BlossomGame& game, cereal::JSONInputArchive* in_archive) {}

	//NECESSARY, display the menu
	virtual void Show(BlossomGame& game) = 0;
};

}