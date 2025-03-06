#include "precompiled/game_precompiled.hpp"
#include <systems/point_of_interest.hpp>

#include <systems/player.hpp>
#include <physics/rigidbody.hpp>
#include <physics/physics_system.hpp>
#include <core/engine.hpp>
#include <core/ecs.hpp>
#include <core/transform.hpp>
#include <glm/glm.hpp>
#include <tools/log.hpp>
#include <systems/basic_particle_system.hpp>
#include <math/geometry.hpp>
#include <systems/collectable.hpp>
#include <core/audio.hpp>

#include <jolt/Jolt.h>
#include <physics/helpers.hpp>
#include <jolt/Physics/Collision/Shape/SphereShape.h>
#include <jolt/Physics/Collision/CollideShape.h>
#include <systems/collisions.hpp>
#include <resources/resource_manager.hpp>
#include <resources/model/model.hpp>
#include <systems/scale_in_system.hpp>
#include <systems/orbiting_bee_system.hpp>
#include <rendering/render_components.hpp>

void bee::POISystem(float dt, int hiveAudioChannelID)
{
	constexpr float DEPOSIT_RANGE_MULT = 0.15f;
	constexpr float POI_START_RANGE_MULT = 0.5f;
	constexpr float POI_SOUND_MULT = 0.5f;

	auto& registry = Engine.ECS().Registry;

	//Get Player
	entt::entity player;
	glm::vec3 playerPos;

	auto playerView = Engine.ECS().Registry.view<Transform, Player>();
	for (auto&& [playerEntity, playerTransform, playerComponent] : playerView.each())
	{
		player = playerEntity;
		playerPos = playerTransform.World()[3];
	}

	auto& playerComponent = registry.get<Player>(player);

	float POIHiveVolume = 0.0f;

	//Case 1: player has no POI
	if (!registry.all_of<Transform, POIComponent>(playerComponent.currentPOI))
	{
		playerComponent.currentPOI = entt::null;
		Engine.Audio().SetChannelVolume(hiveAudioChannelID, 0.0f);

		//Pick a new POI if close enough
		auto POIView = registry.view<POIComponent, Transform>();
		for (auto&& [e, poi, transform] : POIView.each())
		{

			glm::vec3 poiPos = transform.World()[3];
			auto distance = glm::length(poiPos - playerPos);

			float hiveSound = POI_SOUND_MULT * glm::log(poi.activationRange / distance);
			POIHiveVolume = glm::max(POIHiveVolume, hiveSound);

			if (poi.completed) continue;

			if (distance < POI_START_RANGE_MULT * poi.activationRange)
			{
				playerComponent.currentPOI = e;
				if (!poi.started)
				{
					poi.started = true;
					StartNewPOI(e);
				}
			}
		}
	}
	//Case 2: player already has POI
	else
	{
		//Check if POI was completed last frame, then unset
		auto& poi = registry.get<POIComponent>(playerComponent.currentPOI);
		if (poi.completed)
		{
			playerComponent.currentPOI = entt::null;
			return;
		}

		//Handle range to player
		auto& transform = registry.get<Transform>(playerComponent.currentPOI);

		glm::vec3 poiPos = transform.World()[3];
		auto distance = glm::length(poiPos - playerPos);

		//Set volume for sound
		float hiveSound = POI_SOUND_MULT * glm::log(poi.activationRange / distance);
		POIHiveVolume = glm::max(POIHiveVolume, hiveSound);

		if (distance < DEPOSIT_RANGE_MULT * poi.activationRange && playerComponent.currentScore > 0)
		{
			DepositPOI(player, playerComponent.currentPOI);
		}
		else if (distance > POI_START_RANGE_MULT * poi.activationRange)
		{
			playerComponent.currentPOI = entt::null;
		}
	}

	Engine.Audio().SetChannelVolume(hiveAudioChannelID, POIHiveVolume);
}

void bee::StartNewPOI(entt::entity poiEntity)
{
	auto& registry = Engine.ECS().Registry;
	auto* transform = registry.try_get<Transform>(poiEntity);
	auto* poiComponent = registry.try_get<POIComponent>(poiEntity);

	if (transform == nullptr || poiComponent == nullptr) return;

	JPH::SphereShape sphere = JPH::SphereShape(poiComponent->activationRange);
	JPH::Mat44 Jtransform = JPH::Mat44::sIdentity();
	Jtransform.SetTranslation(GlmToJolt(transform->GetTranslation()));

	JPH::CollideShapeSettings collideSettings;

	CollectableCollisionCollector collector;

	Engine.PhysicsSystem()
		.GetNarrowPhaseQuery()
		.CollideShape(&sphere, JPH::Vec3Arg(1.0f, 1.0f, 1.0f), Jtransform, collideSettings, JPH::RVec3Arg(0.0f, 0.0f, 0.0f), collector);
	
	auto& bodyInterface = Engine.PhysicsSystem().GetBodyInterface();
	
	ResourceHandle<Mesh> particleMesh;
	ResourceHandle<Material> particleMaterial;

	if (auto model = poiComponent->particleModel.Retrieve())
	{
		particleMesh = model->meshes.front().front().primitiveMaterialPairs.front().first;
		particleMaterial = model->materials.front();
	}

	for (auto bodyID : collector.collidedBodies) {

		auto entity = static_cast<entt::entity>(bodyInterface.GetUserData(bodyID));

		auto* collectable = registry.try_get<Collectable>(entity);

		if (collectable)
		{
			collectable->isActive = true;

			//Setup particle emitters
			auto& particleEmitter = registry.emplace<EmitterComponent>(entity);

			particleEmitter.cycleTimer = 0.2f;
			particleEmitter.particleDir = World::UP * 2.0f;
			particleEmitter.particleGravity = {};
			particleEmitter.particleLifetime = 2.0f;
			particleEmitter.particleInitialVelocity = 60.0f;
			particleEmitter.particlesPerCycle = 1;
			particleEmitter.particleSpawnOffset = World::UP;
			particleEmitter.particleScale = 0.05f;

			particleEmitter.particleMesh = particleMesh;
			particleEmitter.particleMaterial = particleMaterial;

		}
	}
}

void bee::DepositPOI(entt::entity player, entt::entity poiEntity)
{
	auto& playerC = Engine.ECS().Registry.get<Player>(player);
	auto& POI = Engine.ECS().Registry.get<POIComponent>(poiEntity);

	auto remainingScore = POI.scoreGoal - POI.currentScore;
	auto depositScore = glm::min(remainingScore, playerC.currentScore);

	POI.currentScore += depositScore;
	playerC.currentScore -= depositScore;

	if (auto* orbitalSpawner = Engine.ECS().Registry.try_get<OrbitalSpawnerComponent>(poiEntity))
	{
		orbitalSpawner->shownObjects = POI.currentScore;
	}
	Engine.Audio().PlaySound("audio/sfxDeliverPollen.wav");

	if (POI.currentScore == POI.scoreGoal)
	{
		POI.completed = true;
		FinishPOI(poiEntity);
	}
}

void bee::FinishPOI(entt::entity poiEntity)
{
	auto& registry = Engine.ECS().Registry;
	auto* poiTransform = registry.try_get<Transform>(poiEntity);
	auto* poiComponent = registry.try_get<POIComponent>(poiEntity);

	if (poiTransform == nullptr || poiComponent == nullptr) return;

	JPH::SphereShape sphere = JPH::SphereShape(poiComponent->activationRange);
	JPH::Mat44 Jtransform = JPH::Mat44::sIdentity();
	Jtransform.SetTranslation(GlmToJolt(poiTransform->GetTranslation()));

	JPH::CollideShapeSettings collideSettings;

	CollectableCollisionCollector collector;

	Engine.PhysicsSystem()
		.GetNarrowPhaseQuery()
		.CollideShape(&sphere, JPH::Vec3Arg(1.0f, 1.0f, 1.0f), Jtransform, collideSettings, JPH::RVec3Arg(0.0f, 0.0f, 0.0f), collector);

	auto& bodyInterface = Engine.PhysicsSystem().GetBodyInterface();
	std::vector<entt::entity> entities;

	for (auto bodyID : collector.collidedBodies) {
		auto entity = static_cast<entt::entity>(bodyInterface.GetUserData(bodyID));
		entities.push_back(entity);
	}

	ResourceHandle<Mesh> particleMesh;
	ResourceHandle<Material> particleMaterial;

	if (auto model = poiComponent->particleModel.Retrieve())
	{
		particleMesh = model->meshes.front().front().primitiveMaterialPairs.front().first;
		particleMaterial = model->materials.front();
	}

	//Disable orbiting bees
	if (auto* orbits = registry.try_get<OrbitalSpawnerComponent>(poiEntity))
	{
		orbits->shownObjects = 0;
	}

	//Burst of particles
	auto& particleEmitter = registry.emplace<EmitterComponent>(poiEntity);

	particleEmitter.particleMesh = particleMesh;
	particleEmitter.particleMaterial = particleMaterial;
	particleEmitter.currentTimer += particleEmitter.cycleTimer;
	particleEmitter.disabled = true; //Does not update timer for one-off burst
	particleEmitter.particlesPerCycle = 500;
	particleEmitter.particleInitialVelocity = 300.0f;
	particleEmitter.particleScale = 0.1f;
	particleEmitter.particleLifetime = 10.0f;
	particleEmitter.particleMesh = particleMesh;
	particleEmitter.particleMaterial = particleMaterial;

	// Play SFX
	Engine.Audio().PlaySound("audio/sfxPOIComplete.wav");

	//Disable collectables and do beautify scene
	for (auto entity : entities) {
		if (registry.all_of<Collectable>(entity)) {
			registry.erase<Collectable>(entity);
		}

		if (registry.all_of<EmitterComponent>(entity)) {
			registry.erase<EmitterComponent>(entity);
		}

		auto* transform = registry.try_get<Transform>(entity);

		// for hidden props for the beautify sequence 
		if (transform && registry.all_of<BeautifyTag>(entity)) 
		{
			std::function<void(entt::entity)> unhide_recurse;
			unhide_recurse = [&registry, &unhide_recurse](entt::entity e)
				{
					auto* t = registry.try_get<Transform>(e);
					if (t) for (auto child : *t)
					{
						if (registry.all_of<TagNoDraw>(child)) registry.erase<TagNoDraw>(child);
						unhide_recurse(child);
					}
				};

			//Unhide props
			for (auto entity : *transform)
			{
				unhide_recurse(entity);
			}

			auto pos = glm::vec3(transform->World()[3]);

			float ditheringDeviation = pcg::rand0_1() * 0.4f + 0.6f;
			auto distanceFromCenter = glm::length(pos - glm::vec3(poiTransform->World()[3]));

			auto& scaler = registry.emplace_or_replace<ScaleUpComponent>(entity);

			scaler.targetScale = 1.5f;
			scaler.currentT -= std::sqrtf(distanceFromCenter * ditheringDeviation);
		}
	}
}
