#pragma once

#include <jolt/Jolt.h>
#include <jolt/Physics/PhysicsSystem.h>

namespace Layers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;

	// need better name for this
	// refers to any object that the player shouldn't collide with
	// like collectable flowers and flowers that are part of the
	// beautification sequence
	// maybe interactable?
	static constexpr JPH::ObjectLayer COLLECTABLE = 2;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 3;
};

namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr JPH::BroadPhaseLayer COLLECTABLE(2);
	static constexpr JPH::uint NUM_LAYERS(3);
};