#include <precompiled/engine_precompiled.hpp>
#include "physics/rigidbody.hpp"

void bee::UpdateRigidBody(RigidBody& rigidBody, bee::Transform& transform, float dt)
{
	rigidBody.velocity += rigidBody.acceleration * dt;
    rigidBody.velocity *= 1.0f - glm::min(rigidBody.damping * dt, 1.0f);

    transform.SetTranslation(transform.GetTranslation() + rigidBody.velocity * dt);

    rigidBody.acceleration = glm::vec3{ 0.0f };
}