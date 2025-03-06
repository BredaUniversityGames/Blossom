#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/entity/fwd.hpp>
#include <resources/resource_handle.hpp>
#include <visit_struct/visit_struct.hpp>

namespace bee
{

struct OrbitalMovementComponent
{
	//Rotation of the ring
	glm::quat ringRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 ringCenter = {};

	//Radius of the orbit
	float orbitSphereRadius = 0.0f;

	//Can be negative for clockwise rotations;
	float orbitSpeed = 1.0f;

	//Radians
	float orbitProgress = 0.0f;

	//whether they are shown
	bool enabled = false;
};

struct OrbitalSpawnerComponent
{
	//Start out with 2 bees rotating around the POI
	uint32_t shownObjects = 2;
	std::vector<entt::entity> orbitingEntities;

	//Model of the orbital particle
	ResourceHandle<Model> particleModel;

	//Spawned evenly across the ring
	uint32_t objectsPerRing = 3;

	//Since we are going to have the rings be symmetrical, we count the pairs that go on each side
	uint32_t ringPairCount = 1; 

	//How much the rings deviate per consequent ring
	glm::vec3 ringEulerSteps{};

	//Replicated to the spawned particles
	OrbitalMovementComponent orbitalProperties{};

	static void SubscribeToEvents();
	static void UnsubscribeToEvents();

private:
	static void OnCreate(entt::registry& registry, entt::entity entity);
	static void OnDestroy(entt::registry& registry, entt::entity entity);
};

void OrbitalParticleMovementSystem(float dt);
void OrbitalSpawnerSystem();

//Drawn under the Gameplay Category
void DebugDrawSpawnerOrbitals();

}

VISITABLE_STRUCT(bee::OrbitalMovementComponent, orbitSphereRadius, orbitSpeed);
VISITABLE_STRUCT(bee::OrbitalSpawnerComponent, particleModel, objectsPerRing, ringPairCount, ringEulerSteps, orbitalProperties);