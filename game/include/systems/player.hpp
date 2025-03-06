#pragma once

#include <vector>
#include <optional>
#include <resources/resource_handle.hpp>
#include <glm/glm.hpp>

namespace bee
{

struct RigidBody;
struct Transform;
struct Displacer;
class Level;

struct Player
{
    // rotation control
    glm::vec3 eulerRadians;
    glm::vec3 pullEulerRadians;

    // speed tracking
    float speed = 0.0f;
    float boostTimer = 0.0f;

    float maxSpeed = 12.0f;
    float boostMaxSpeed = 28.0f;

    float acceleration = 0.5f;
    float boostedAcc = 0.9f;
    float decceleration = -0.5f;

    float boostActiveTime = 0.9f;
    float boostResetTime = 0.5f;
    float boostGracePeriod = -0.5f;

    // turning variables
    // These need better names

    // multiplied by input rotation added to pullEulerRadians
    float turnSpeed = 1.3f;
    // if distance between pullEulerRadians and radians is greater than this, eulerRadians will be set to this distance apart
    float maxAngleDelta = 0.4f;
    // brings the pullEulerRadians back towards the eulerRadians
    float turnResetModifier = 0.45f;

    // collision data
    float playerCollectionRadius = 1.5f;
    float playerCollisionRadius = 0.5f;
    entt::entity currentPOI;

    // score tracking
    int currentScore{};
    int scoreCap = 10;
};

void UpdatePlayerControlInversion();

void PlayerMovementSystem(std::shared_ptr<Level> level, float dt);

}

VISITABLE_STRUCT(bee::Player,
    maxSpeed,
    boostMaxSpeed,
    acceleration,
    boostedAcc,
    decceleration,
    turnSpeed,
    boostActiveTime,
    boostResetTime,
    boostGracePeriod,
    playerCollectionRadius,
    currentScore,
    scoreCap
);
