#include "precompiled/game_precompiled.hpp"
#include <systems/scale_in_system.hpp>

#include <core/engine.hpp>
#include <core/transform.hpp>
#include <core/ecs.hpp>

#include <math/easing.hpp>

void bee::ScaleUpSystem(float dt)
{
	auto view = Engine.ECS().Registry.view<Transform, ScaleUpComponent>();

	for (auto&& [e, transform, scaleup] : view.each())
	{
		scaleup.currentT += dt * scaleup.speed;
		
		float scaleT = glm::clamp(scaleup.currentT, 0.0f, 1.0f);
		float easedT = Ease::EaseOutBack(scaleT);
		float scaleVal = scaleup.initialScale + easedT * (scaleup.targetScale - scaleup.initialScale);

		transform.SetScale(glm::vec3(scaleVal));
	}
}
