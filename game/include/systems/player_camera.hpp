#pragma once

#include <glm/glm.hpp>
#include <visit_struct/visit_struct.hpp>
#include "core/transform.hpp"
#include "rendering/render_components.hpp"

namespace bee
{

void PlayerCameraSystem(float dt);
void FreeCameraSystem(float dt);

struct PlayerCamera
{
    entt::entity player{};

    float tFOV = 0.0f;
    float FOVDeltaMul = 1.3f;
    float minFOV = 0.25f * glm::pi<float>();
    float maxFOV = 0.35f * glm::pi<float>();

    glm::vec3 eulerRadians{0.0f};
    glm::vec3 pullEulerRadiansDelta{0.0f};
    glm::vec3 followOffset = glm::vec3(0.0f, -3.0f, 0.4f);

    // camera deviation stuff
    glm::vec3 maxDeviation = glm::vec3(0.8f, 0.0f, 0.5f);
    glm::vec3 deviationDeltaMultiplier = glm::vec3(2.0f, 1.0f, 1.0f);
    glm::vec3 pullDeviationTo = glm::vec3(0.5f, 0.0f, 0.55f);
    glm::vec3 pullStrength = glm::vec3(-0.4f, 0.0f, 0.6f);
    glm::vec3 tDeviation{0.0f};
};

} // namespace bee

VISITABLE_STRUCT(bee::PlayerCamera,
    FOVDeltaMul,
    minFOV,
    maxFOV,
    eulerRadians,
    followOffset,
    maxDeviation,
    deviationDeltaMultiplier
);
