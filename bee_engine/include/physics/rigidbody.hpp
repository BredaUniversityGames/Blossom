#pragma once

#include <glm/vec3.hpp>
#include "core/transform.hpp"
#include <visit_struct/visit_struct.hpp>

namespace bee {

struct RigidBody
{
    glm::vec3 velocity{ 0.0f };
    glm::vec3 acceleration{ 0.0f };

    float damping{ 0.5f };
    float maxVelocity{ 2.0f };
};

void UpdateRigidBody(RigidBody& rigidBody, bee::Transform& transform, float dt);

}

VISITABLE_STRUCT(bee::RigidBody, velocity, acceleration, damping, maxVelocity);
