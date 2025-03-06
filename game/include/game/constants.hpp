#pragma once
#include <vector>
#include <string>

namespace bee
{

//A couple constants for the game like paths for assets
//They were placed in this file to avoid having hardcoded paths all over the place
//Now we only have them in one spot
struct GameConstants
{
	//All keyframes of the player animation
	std::vector<std::string> playerAnimations{
		"models/player/BeeAnim1.gltf",
		"models/player/BeeAnim2.gltf",
		"models/player/BeeAnim3.gltf",
		"models/player/BeeAnim4.gltf",
		"models/player/BeeAnim5.gltf"
	};

	//Order in which the player animations are player
	std::vector<size_t> animationOrdering{
		0, 1, 2, 3, 4, 3, 2, 1
	};

	//Victory Text
	std::string victoryTextPath = "ui/WinMessage.png";
	float victoryTextSize = 0.20f;

	//UI play button
	std::string playButton = "ui/PlayButton.png";
	std::string playButtonHovered = "ui/PlayButtonHovered.png";

	//UI reset button
	std::string resetButton = "ui/ResetButton.png";
	std::string resetButtonHovered = "ui/ResetButtonHovered.png";

	//UI exit button
	std::string exitButton = "ui/ExitButton.png";
	std::string exitButtonHovered = "ui/ExitButtonHovered.png";

	//Hive Honey bar
	std::string hiveHoneyBar = "ui/HiveIndicator/HiveForeground.png";
	std::string hiveHoneyBack = "ui/HiveIndicator/HiveBackground.png";
	std::string hiveHoneyDynamic = "ui/HiveIndicator/HiveProgress.png";

	// Pollen bar
	std::string playerHoneyBar = "ui/BeeIndicator/BeeForeground.png";
	std::string playerHoneyBack = "ui/BeeIndicator/BeeBackground.png";
	std::string playerHoneyDynamic = "ui/BeeIndicator/BeeProgress.png";

	

};


}