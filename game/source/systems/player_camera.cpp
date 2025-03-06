#include <precompiled/game_precompiled.hpp>
#include "systems/player_camera.hpp"

#include "core/engine.hpp"
#include "core/input.hpp"
#include <tools/log.hpp>

#include <core/ecs.hpp>
#include "systems/player.hpp"
#include <core/transform.hpp>
#include "math/easing.hpp"

#include <systems/input_helpers.hpp>

namespace bee
{

void UpdateCameraDeviation(PlayerCamera& camera, const Player& player, float dt)
{
    glm::vec3 pullEulerDiff = player.pullEulerRadians - player.eulerRadians;
    camera.tDeviation.x -= pullEulerDiff.z * camera.deviationDeltaMultiplier.x * dt;
    camera.tDeviation.x = glm::clamp(camera.tDeviation.x, 0.0f, 1.0f);

    camera.tDeviation.z += pullEulerDiff.x * camera.deviationDeltaMultiplier.z * dt;
    camera.tDeviation.z = glm::clamp(camera.tDeviation.z, 0.0f, 1.0f);

    glm::vec3 pullDeviation = camera.pullDeviationTo - camera.tDeviation;
    pullDeviation = pullDeviation * (camera.pullStrength * dt);

    camera.tDeviation += pullDeviation;
    camera.tDeviation = glm::clamp(camera.tDeviation, 0.0f, 1.0f);
}

void PlayerCameraSystem(float dt)
{
    constexpr float stickRotationSpeed = 1.2f;
    constexpr int defaultGamepad = 0;

    std::optional<int> usedGamepad{};
    if (Engine.Input().IsGamepadAvailable(defaultGamepad) && Engine.Input().IsGamepadActive()) usedGamepad = defaultGamepad;

    glm::vec3 inputRotation{};

    //if (usedGamepad) 
    //{
    //    inputRotation = GetInputEulerRotation(usedGamepad, Stick::Right, stickRotationSpeed * dt);
    //}

    auto cameras = Engine.ECS().Registry.view<Transform, PlayerCamera, CameraComponent>();

    for (auto&& [entity, transform, camera, cameraSettings] : cameras.each()) {
        if (!Engine.ECS().Registry.valid(camera.player)) continue;

        auto* playerTransform = Engine.ECS().Registry.try_get<Transform>(camera.player);
        auto* player = Engine.ECS().Registry.try_get<Player>(camera.player);

        if (player == nullptr || playerTransform == nullptr) continue;

        glm::vec3 eulerDiff = player->eulerRadians - camera.eulerRadians;

        camera.eulerRadians.z = player->eulerRadians.z;
        camera.eulerRadians.x = player->eulerRadians.x;

        float targetFOV = player->speed / player->boostMaxSpeed;

        camera.tFOV += (targetFOV - camera.tFOV) * camera.FOVDeltaMul * dt;
        cameraSettings.fieldOfView = camera.minFOV + (camera.maxFOV - camera.minFOV) * Ease::SmoothStep(camera.tFOV);

        UpdateCameraDeviation(camera, *player, dt);

        camera.pullEulerRadiansDelta += inputRotation;
        camera.pullEulerRadiansDelta -= camera.pullEulerRadiansDelta * Ease::OutPow(glm::min(dt, 1.0f), 2);

        glm::quat rotation = glm::quat(camera.eulerRadians + camera.pullEulerRadiansDelta);
        glm::vec3 tDeviation = glm::vec3(Ease::SmoothStep(camera.tDeviation.x), Ease::SmoothStep(camera.tDeviation.y), Ease::SmoothStep(camera.tDeviation.z));
        glm::vec3 offset = rotation * (camera.followOffset + (tDeviation * camera.maxDeviation * 2.0f - camera.maxDeviation));

        transform.SetTranslation(playerTransform->GetTranslation() + offset);
        transform.SetRotation(rotation);
    }
}

void FreeCameraSystem(float dt)
{
    //If appropriate, these constants should be moved to component variables in the ECS

    constexpr int defaultGamepad = 0;
    constexpr float stickRotationSpeed = 1.2f;
    constexpr float mouseSensitivity = 0.0005f;
    constexpr float cameraSpeed = 20.0f;

    //Selects default gamepad or null if there is no gamepad
    std::optional<int> usedGamepad{};
    if (Engine.Input().IsGamepadAvailable(defaultGamepad) && Engine.Input().IsGamepadActive()) usedGamepad = defaultGamepad;

    glm::vec3 eulerDelta{};
    if (usedGamepad) {
        eulerDelta = GetInputEulerRotation(usedGamepad, Stick::Right, stickRotationSpeed * dt);
    }
    else {
        eulerDelta = GetInputEulerRotation(std::nullopt, Stick::Right, mouseSensitivity);
    }
    
    glm::vec3 movementVector = GetInputMovementDirection(usedGamepad);

    auto cameras = Engine.ECS().Registry.view<Transform, PlayerCamera>();

    for (auto&& [entity, transform, camera] : cameras.each()) {
 
        camera.eulerRadians += eulerDelta;
        camera.eulerRadians.x = glm::clamp(camera.eulerRadians.x, glm::radians(-89.9f), glm::radians(89.9f));

        glm::quat rotation = glm::quat(camera.eulerRadians);

        transform.SetRotation(rotation);
        transform.SetTranslation(transform.GetTranslation() + (rotation * movementVector) * dt * cameraSpeed);
    }
}

} // namespace bee