#pragma once
#include <core/ecs.hpp>
#include <ui/ui.hpp>
#include <resources/resource_handle.hpp>

namespace bee
{

//Helper struct for storing entities to UI bar elements
struct DynamicBar
{
	entt::entity background = entt::null;
	entt::entity dynamicBar = entt::null;
	entt::entity foreground = entt::null;
	bool horizontal = true;
	float margin = 0.0f;

	DynamicBar() = default;

	DynamicBar(
		entt::registry& registry,
		ResourceHandle<Image> backgroundTexture,
		ResourceHandle<Image> dynamicTexture,
		ResourceHandle<Image> foregroundTexture,
		UIElement::Origin origin,
		glm::vec2 position,
		glm::vec2 scale,
		bool isHorizontal,
		float margin
	);

	void SetFill(entt::registry& registry, float ratio);

	void SetVisible(entt::registry& registry, bool active);
};

}