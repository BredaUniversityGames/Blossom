#include "precompiled/game_precompiled.hpp"
#include "systems/playerHUDhelpers.hpp"

#include <math/math.hpp>

bee::DynamicBar::DynamicBar(entt::registry& registry, ResourceHandle<Image> backgroundTexture, ResourceHandle<Image> dynamicTexture, ResourceHandle<Image> foregroundTexture, UIElement::Origin origin, glm::vec2 position, glm::vec2 scale, bool isHorizontal, float side_margin)
{
    horizontal = isHorizontal;
    margin = side_margin;

    background = registry.create();
    dynamicBar = registry.create();
    foreground = registry.create();

    auto& bUI = registry.emplace<UIElement>(background);
    auto& dUI = registry.emplace<UIElement>(dynamicBar);
    auto& fUI = registry.emplace<UIElement>(foreground);

    bUI.texture = backgroundTexture; bUI.layer = -1;
    dUI.texture = dynamicTexture; dUI.layer = 0;
    fUI.texture = foregroundTexture; fUI.layer = 1;

    std::array<UIElement*, 3> elements = { &bUI, &dUI, &fUI };

    for (auto elem : elements)
    {
        elem->draw_on_main_menu = false;
        elem->origin = origin;
        elem->position = position;
        elem->scale = scale;
    }

    if (horizontal)
    {
        dUI.position.x += margin;
        dUI.scale.x = 0.0f;
    }
    else
    {
        dUI.position.y += margin;
        dUI.scale.y = 0.0f;
    }
}

void bee::DynamicBar::SetFill(entt::registry& registry, float ratio)
{
    constexpr float EASE_FACTOR = 0.2f;

    auto& dynamic = registry.get<UIElement>(dynamicBar);
    auto& bar = registry.get<UIElement>(foreground);

    float maxSize{};
    if (horizontal)
    {
        maxSize = bar.scale.x;
    }
    else
    {
        maxSize = bar.scale.y;
    }

    float setSize = (maxSize) * ratio;

    if (horizontal)
    {
        dynamic.scale.x = Lerp(dynamic.scale.x, setSize, EASE_FACTOR);
    }
    else
    {
        dynamic.scale.y = Lerp(dynamic.scale.y, setSize, EASE_FACTOR);
    }
}

void bee::DynamicBar::SetVisible(entt::registry& registry, bool active)
{
    registry.get<UIElement>(background).visible = active;
    registry.get<UIElement>(dynamicBar).visible = active;
    registry.get<UIElement>(foreground).visible = active;
}
