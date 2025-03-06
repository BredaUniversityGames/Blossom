#include "precompiled/game_precompiled.hpp"
#include <core/ecs.hpp>
#include <core/engine.hpp>
#include <core/transform.hpp>
#include <systems/model_root_component.hpp>
#include <resources/model/model.hpp>

void bee::ModelRootComponent::SubscribeToEvents()
{
	Engine.ECS().Registry.on_update<ModelRootComponent>().connect<ModelRootComponent::OnPatch>();
	Engine.ECS().Registry.on_construct<ModelRootComponent>().connect<ModelRootComponent::OnPatch>();
	Engine.ECS().Registry.on_destroy<ModelRootComponent>().connect<ModelRootComponent::OnDestroy>();
}

void bee::ModelRootComponent::UnsubscribeToEvents()
{
	Engine.ECS().Registry.on_update<ModelRootComponent>().disconnect<ModelRootComponent::OnPatch>();
	Engine.ECS().Registry.on_construct<ModelRootComponent>().disconnect<ModelRootComponent::OnPatch>();
	Engine.ECS().Registry.on_destroy<ModelRootComponent>().disconnect<ModelRootComponent::OnDestroy>();
}

void bee::ModelRootComponent::OnPatch(entt::registry& registry, entt::entity entity)
{
	OnDestroy(registry, entity);

	auto& model = registry.get<ModelRootComponent>(entity);
	if (auto m = model.model.Retrieve()) {
		m->InstantiateScene(registry, entity);
	}
}

void bee::ModelRootComponent::OnDestroy(entt::registry& registry, entt::entity entity)
{
	auto* transform = registry.try_get<Transform>(entity);
	if (transform == nullptr) return;
	
	for (auto child : *transform) {
		Engine.ECS().DeleteEntity(child);
	}

	transform->DetachChildren(registry);
}
