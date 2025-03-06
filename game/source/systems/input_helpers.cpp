#include <precompiled/game_precompiled.hpp>
#include <systems/input_helpers.hpp>
#include <tools/log.hpp>
#include <core/engine.hpp>
#include <core/input.hpp>
#include <glm/gtc/quaternion.hpp>

glm::vec3 bee::GetInputEulerRotation(std::optional<int> gamepadId, Stick stick, float sensitivity)
{
    glm::vec3 eulerDelta{};

    if (gamepadId)
    {
        switch (stick)
        {
        case Stick::Left:
        {
            eulerDelta.x -= -Engine.Input().GetGamepadAxis(gamepadId.value(), Input::GamepadAxis::StickLeftY) * sensitivity;
            eulerDelta.z += -Engine.Input().GetGamepadAxis(gamepadId.value(), Input::GamepadAxis::StickLeftX) * sensitivity;
        } 
        break;
        case Stick::Right:
        {
            eulerDelta.x -= -Engine.Input().GetGamepadAxis(gamepadId.value(), Input::GamepadAxis::StickRightY) * sensitivity;
            eulerDelta.z += -Engine.Input().GetGamepadAxis(gamepadId.value(), Input::GamepadAxis::StickRightX) * sensitivity;
        }
        break;
        }        
    }
    else
    {
        glm::vec2 cursorVector = Engine.Input().GetMouseMovement();
        eulerDelta.x += -cursorVector.y * sensitivity;
        eulerDelta.z += -cursorVector.x * sensitivity;
    }

    return eulerDelta;
}

glm::vec3 bee::GetInputMovementDirection(std::optional<int> gamepadId)
{
    glm::vec3 movementVector{};

    if (gamepadId)
    {
        movementVector.y = -Engine.Input().GetGamepadAxis(gamepadId.value(), Input::GamepadAxis::StickLeftY);
        movementVector.x = Engine.Input().GetGamepadAxis(gamepadId.value(), Input::GamepadAxis::StickLeftX);

        if (Engine.Input().GetGamepadButton(gamepadId.value(), Input::GamepadButton::South))
            movementVector.z += 1;

        if (Engine.Input().GetGamepadButton(gamepadId.value(), Input::GamepadButton::East))
            movementVector.z -= 1;
    }
    else
    {
        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::W))
            movementVector.y += 1;

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::S))
            movementVector.y -= 1;

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::A))
            movementVector.x -= 1;

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::D))
            movementVector.x += 1;

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::LeftShift))
            movementVector.z -= 1;

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::Space))
            movementVector.z += 1;
    }

    if (glm::length(movementVector) > 0.0f)
    {
        movementVector = glm::normalize(movementVector);
    }

    return movementVector;
}
