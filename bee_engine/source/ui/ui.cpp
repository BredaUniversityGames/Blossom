#include <precompiled/engine_precompiled.hpp>
#include "ui/ui.hpp"

#include "core/engine.hpp"
#include "core/device.hpp"
#include "core/input.hpp"
#include "core/ecs.hpp"
#include "core/transform.hpp"

bool PointRectCollision(glm::vec2 pointPos, glm::vec2 rectCenterPos, glm::vec2 rectDim)
{
    float halfWidth = rectDim.x * 0.5f;
    bool leftOverlap  = pointPos.x > rectCenterPos.x - halfWidth;
    bool rightOverlap = pointPos.x < rectCenterPos.x + halfWidth;

    float halfHeight = rectDim.y * 0.5f;
    bool topOverlap    = pointPos.y < rectCenterPos.y + halfHeight;
    bool bottomOverlap = pointPos.y > rectCenterPos.y - halfHeight;

    return leftOverlap && rightOverlap && topOverlap && bottomOverlap;
}

void SelectButton(bee::UIMenu& menu, int gamepadID, float dt)
{
    if (bee::Engine.Input().IsGamepadActive() && bee::Engine.Input().IsGamepadAvailable(gamepadID))
    {
        if (menu.buttonIndex == -1)
        {
            menu.buttonIndex = menu.prevButtonIndex;
        }

        float stickEpsilon = 0.2f;
        float startScrolling = 2.0f;
        float scrollInterval = 1.0f;

        float prevStickVal = bee::Engine.Input().GetGamepadAxisPrevious(gamepadID, bee::Input::GamepadAxis::StickLeftY);
        float stickVal = bee::Engine.Input().GetGamepadAxis(gamepadID, bee::Input::GamepadAxis::StickLeftY);
        int direction = (int)(abs(stickVal) / stickVal);

        menu.prevButtonIndex = menu.buttonIndex;

        if (abs(stickVal) > stickEpsilon)
        {
            if (abs(prevStickVal) <= stickEpsilon || (int)(abs(prevStickVal) / prevStickVal) != direction)
            {
                menu.stickTimer = 0.0f;
                menu.scrolling = false;
                
                menu.buttonIndex += direction;
            }

            menu.stickTimer += dt;

            if (!menu.scrolling)
            {
                if (menu.stickTimer > startScrolling)
                {
                    menu.buttonIndex += direction;
                    menu.stickTimer -= startScrolling;
                    menu.scrolling = true;
                }
            }
            else
            {
                if (menu.stickTimer > scrollInterval)
                {
                    menu.buttonIndex += direction;
                    menu.stickTimer -= scrollInterval;
                }
            }
        }

        if (bee::Engine.Input().GetGamepadButtonOnce(gamepadID, bee::Input::GamepadButton::DPadDown))
        {
            menu.buttonIndex += 1;
        }

        if (bee::Engine.Input().GetGamepadButtonOnce(gamepadID, bee::Input::GamepadButton::DPadUp))
        {
            menu.buttonIndex -= 1;
        }

        menu.buttonIndex = std::clamp(menu.buttonIndex, 0, (int)menu.buttons.size() - 1);
    }
    else
    {
        glm::vec2 mousePos = bee::Engine.Input().GetProjectedMousePosition();

        bool buttonCollided = false;

        for (int i = 0; i < menu.buttons.size(); i += 1)
        {
            bee::Button& button = menu.buttons[i];

            glm::vec2 buttonDim = glm::vec2(button.scale, button.inverseAspect * button.scale);
            bool collision = PointRectCollision(mousePos, button.position, buttonDim);

            if (collision)
            {
                buttonCollided = true;
                
                if (menu.buttonIndex != -1)
                {
                    menu.prevButtonIndex = menu.buttonIndex;
                }
                menu.buttonIndex = i;

                return;
            }
        }

        if (!buttonCollided)
        {
            menu.buttonIndex = -1;
        }
    }
}

void bee::ui::Update(float dt)
{
    auto menus = Engine.ECS().Registry.view<UIMenu>();

    if (menus.empty())
    {
        return;
    }

    auto menu_ent = menus.front();
    UIMenu& menu = menus.get<UIMenu>(menu_ent);

    int gamepadID = 0;
    SelectButton(menu, gamepadID, dt);

    // update highlighted button
    
    for (auto& button : menu.buttons)
    {
        button.texture = button.plainTex;
    }
    
    //menu.buttons[menu.prevButtonIndex].texture = menu.buttons[menu.prevButtonIndex].plainTex;
    if (menu.buttonIndex != -1)
    {   
        menu.buttons[menu.buttonIndex].texture = menu.buttons[menu.buttonIndex].hoveredTex;
    }

    // run button function if clicked
    if ((Engine.Input().GetMouseButtonOnce(Input::MouseButton::Left) || Engine.Input().GetGamepadButtonOnce(gamepadID, Input::GamepadButton::South)) && menu.buttonIndex != -1)
    {
        menu.buttons[menu.buttonIndex].callback(menu_ent);
    }
}
