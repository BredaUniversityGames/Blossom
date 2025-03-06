#pragma once
#include <glm/glm.hpp>
#include <optional>

namespace bee {

enum Stick
{
	Left,
	Right
};

//Specifying a gamepad will override mouse input
glm::vec3 GetInputEulerRotation(std::optional<int> gamepadId, Stick stick, float sensitivity);

//Specifying a gamepad will override keyboard
glm::vec3 GetInputMovementDirection(std::optional<int> gamepadId);

}