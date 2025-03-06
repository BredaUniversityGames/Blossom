#pragma once

#include "resources/resource_handle.hpp"
#include <glm/glm.hpp>
#include <functional>
#include <entt/entity/entity.hpp>
#include <vector>

namespace bee
{

class Shader;
class Game;

struct Button
{
	ResourceHandle<Image> texture;
	ResourceHandle<Image> plainTex;
    ResourceHandle<Image> hoveredTex;
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec2 position{};
	float inverseAspect = 1.0f;
	float scale = 1.0f;
	std::function<void(entt::entity)> callback;
};

struct UIMenu
{
	std::vector<Button> buttons;
	ResourceHandle<Mesh> mesh;
	int prevButtonIndex = 0;
	int buttonIndex = 0;
	float stickTimer = 0.0f;
	bool scrolling = false;
};

struct UIElement
{
	enum class Origin
	{
		CENTRE,
		NORTH_WEST,
		NORTH,
		NORTH_EAST,
		EAST,
		SOUTH_EAST,
		SOUTH,
		SOUTH_WEST,
		WEST
	};

	ResourceHandle<Image> texture;

	Origin origin = Origin::CENTRE;
	glm::vec2 position = glm::vec2(0.0f);
	glm::vec2 scale = glm::vec2(1.0f);

	bool draw_on_main_menu = false;
	bool visible = true;
    int layer = 0;
};

class UIRenderer
{
public:
	UIRenderer();
	~UIRenderer();

	NON_COPYABLE(UIRenderer);
	NON_MOVABLE(UIRenderer);

	void Render();

private:
	glm::vec2 CalculateQuadOffset(glm::vec2 quadSize, UIElement::Origin origin)
	{
        glm::vec2 halfQuadSize = quadSize / 2.0f;

        switch (origin)
        {
            case UIElement::Origin::NORTH_WEST:
            {
                return glm::vec2{ halfQuadSize.x, -halfQuadSize.y };
                break;
            }
            case UIElement::Origin::NORTH:
            {
                return {0.0f, -halfQuadSize.y};
                break;
            }
            case UIElement::Origin::NORTH_EAST:
            {
                return { -halfQuadSize.x, -halfQuadSize.y };
                break;
            }
            case UIElement::Origin::EAST:
            {
                return { -halfQuadSize.x, 0.0f };
                break;
            }
            case UIElement::Origin::SOUTH_EAST:
            {
                return { -halfQuadSize.x, halfQuadSize.y };
                break;
            }
            case UIElement::Origin::SOUTH:
            {
                return{ 0.0f, halfQuadSize.y };
                break;
            }
            case UIElement::Origin::SOUTH_WEST:
            {
                return { halfQuadSize.x, halfQuadSize.y };
                break;
            }
            case UIElement::Origin::WEST:
            {
                return { halfQuadSize.x, 0.0f };
                break;
            }
            default:
            {
                return {};
                break;
            }
        }
	}

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

	std::shared_ptr<Shader> m_uiPass;
	ResourceHandle<Mesh> m_quadMesh;
};

namespace ui
{
	void Update(float dt);
	void Render(std::shared_ptr<Shader> uiShader);

}


}


VISITABLE_STRUCT(bee::UIElement, position, scale, draw_on_main_menu);