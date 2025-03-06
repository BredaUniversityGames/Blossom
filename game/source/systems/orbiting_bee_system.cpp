#include "precompiled/game_precompiled.hpp"
#include "systems/orbiting_bee_system.hpp"

#include <rendering/render_components.hpp>
#include <core/transform.hpp>
#include <rendering/debug_render.hpp>
#include <core/engine.hpp>
#include <core/ecs.hpp>
#include <tools/log.hpp>

#include <math/geometry.hpp>
#include <math/math.hpp>
#include <systems/model_root_component.hpp>

void bee::OrbitalParticleMovementSystem(float dt)
{
	constexpr float WAVE_MOVEMENT_AMPLITUDE = 0.1f;
	constexpr float WAVE_MOVEMENT_FREQUENCY = 5.0f;
	constexpr float BEE_SPEED = 1.0f;
	constexpr glm::vec3 RED = { 1.0f, 0.0f, 0.0f };

	auto& registry = Engine.ECS().Registry;
	auto view = registry.view<OrbitalMovementComponent, Transform>();

	for (auto&& [e, orbital, transform] : view.each())
	{
		orbital.orbitProgress += dt * orbital.orbitSpeed;

		if (orbital.enabled)
		{
			glm::vec2 circlePoint = glm::vec2(std::cosf(orbital.orbitProgress), std::sinf(orbital.orbitProgress));
			float zOffset = std::sinf(orbital.orbitProgress * WAVE_MOVEMENT_FREQUENCY) * WAVE_MOVEMENT_AMPLITUDE;

			glm::vec3 adjustedOrbitPoint = orbital.ringRotation * glm::vec3(circlePoint * orbital.orbitSphereRadius, zOffset);
			glm::vec3 desiredPoint = orbital.ringCenter + adjustedOrbitPoint;

			glm::vec3 desiredVector = desiredPoint - transform.Translation;
			glm::quat rotation = glm::conjugate(glm::quat(glm::lookAt(desiredVector, { 0.0f, 0.0f, 0.0f }, -World::UP)));

			transform.SetRotation(rotation * World::YUP_TO_ZUP);
			transform.SetTranslation(bee::Lerp(transform.Translation, desiredPoint, 0.1f));
		}
		else
		{
			transform.SetTranslation(orbital.ringCenter);
		}
	}
}

void bee::OrbitalSpawnerSystem()
{
	auto& registry = Engine.ECS().Registry;
	auto view = registry.view<OrbitalSpawnerComponent>();

	for (auto&& [e, orbital] : view.each())
	{
		size_t i = 0;
		for (auto entity : orbital.orbitingEntities)
		{
			auto* comp = registry.try_get<OrbitalMovementComponent>(entity);
			if (i < orbital.shownObjects && comp)
			{
				comp->enabled = true;
			}
			else if (comp)
			{
				comp->enabled = false;
			}
			i++;
		}
	}
}

void bee::DebugDrawSpawnerOrbitals()
{
	constexpr glm::vec4 GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };

	auto& registry = Engine.ECS().Registry;
	auto view = registry.view<OrbitalSpawnerComponent, Transform>();

	for (auto&& [e, spawner, transform] : view.each())
	{
		glm::vec3 position = transform.World()[3];

		for (uint32_t i = 0; i < spawner.ringPairCount; i++)
		{
			//Start with two centered rings and then add steps on that for more rotation
			auto eulerIncr = spawner.ringEulerSteps * static_cast<float>(i) +
				spawner.ringEulerSteps * 0.5f;

			auto rotation1 = glm::quat(eulerIncr);
			auto rotation2 = glm::quat(-eulerIncr);

			Engine.DebugRenderer()
				.AddCircle(DebugCategory::Gameplay, position, rotation1, spawner.orbitalProperties.orbitSphereRadius, GREEN);

			Engine.DebugRenderer()
				.AddCircle(DebugCategory::Gameplay, position, rotation2, spawner.orbitalProperties.orbitSphereRadius, GREEN);
		}
	}
}

void bee::OrbitalSpawnerComponent::SubscribeToEvents()
{
	Engine.ECS().Registry.on_update<OrbitalSpawnerComponent>().connect<OrbitalSpawnerComponent::OnCreate>();
	Engine.ECS().Registry.on_construct<OrbitalSpawnerComponent>().connect<OrbitalSpawnerComponent::OnCreate>();
	Engine.ECS().Registry.on_destroy<OrbitalSpawnerComponent>().connect<OrbitalSpawnerComponent::OnDestroy>();
}

void bee::OrbitalSpawnerComponent::UnsubscribeToEvents()
{
	Engine.ECS().Registry.on_update<OrbitalSpawnerComponent>().disconnect<OrbitalSpawnerComponent::OnCreate>();
	Engine.ECS().Registry.on_construct<OrbitalSpawnerComponent>().disconnect<OrbitalSpawnerComponent::OnCreate>();
	Engine.ECS().Registry.on_destroy<OrbitalSpawnerComponent>().disconnect<OrbitalSpawnerComponent::OnDestroy>();
}

void bee::OrbitalSpawnerComponent::OnCreate(entt::registry& registry, entt::entity entity)
{
	constexpr glm::vec3 PARTICLE_SCALE = { 0.3f, 0.3f, 0.3f };
	constexpr float ORBIT_RING_PROGRESS_INCR = 1.0f;

	OnDestroy(registry, entity);

	auto& spawner = registry.get<OrbitalSpawnerComponent>(entity);
	glm::vec3 position = {};

	auto* transform = registry.try_get<Transform>(entity);
	if (transform)
	{
		position = transform->World()[3];
	}

	//Create all particles, hidden for now
	for (uint32_t ringPair = 0; ringPair < spawner.ringPairCount; ringPair++)
	{
		//Start with two centered rings and then add steps on that for more rotation
		auto eulerIncr = spawner.ringEulerSteps * static_cast<float>(ringPair) +
			spawner.ringEulerSteps * 0.5f;

		auto rotation1 = glm::quat(eulerIncr);
		auto rotation2 = glm::quat(-eulerIncr);

		float radianIncr = ringPair * ORBIT_RING_PROGRESS_INCR;

		//First Ring (+)
		for (uint32_t objectI = 0; objectI < spawner.objectsPerRing; objectI++)
		{
			float radiansStart = objectI * (2.0f * glm::pi<float>() / static_cast<float>(spawner.objectsPerRing))
				+ radianIncr;
			
			auto new_entity = registry.create();

			auto& new_transform = registry.emplace<Transform>(new_entity);
			new_transform.Scale = PARTICLE_SCALE;

			auto& new_model = registry.emplace<ModelRootComponent>(new_entity, spawner.particleModel);
			auto& new_orbital = registry.emplace<OrbitalMovementComponent>(new_entity);

			new_orbital = spawner.orbitalProperties;
			new_orbital.ringCenter = position;
			new_orbital.orbitProgress = radiansStart;
			new_orbital.ringRotation = rotation1;

			spawner.orbitingEntities.emplace_back(new_entity);
		}

		//Second Ring (-)
		for (uint32_t objectI = 0; objectI < spawner.objectsPerRing; objectI++)
		{
			float radiansStart = -(objectI * (2.0f * glm::pi<float>() / static_cast<float>(spawner.objectsPerRing)))
				+ radianIncr;

			auto new_entity = registry.create();
			auto& new_transform = registry.emplace<Transform>(new_entity);
			new_transform.Scale = PARTICLE_SCALE;

			auto& new_model = registry.emplace<ModelRootComponent>(new_entity, spawner.particleModel);
			auto& new_orbital = registry.emplace<OrbitalMovementComponent>(new_entity);

			new_orbital = spawner.orbitalProperties;
			new_orbital.orbitProgress = radiansStart;
			new_orbital.ringCenter = position;
			new_orbital.ringRotation = rotation2;
			new_orbital.orbitSpeed *= -1.0f;

			spawner.orbitingEntities.emplace_back(new_entity);
		}
	}
}

void bee::OrbitalSpawnerComponent::OnDestroy(entt::registry& registry, entt::entity entity)
{
	auto& spawner = registry.get<OrbitalSpawnerComponent>(entity);

	for (auto e : spawner.orbitingEntities)
	{
		if (registry.valid(e)) Engine.ECS().DeleteEntity(e);
	}
}
