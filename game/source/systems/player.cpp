#include <precompiled/game_precompiled.hpp>
#include "systems/player.hpp"

#include <jolt/Jolt.h>
#include <jolt/Physics/Collision/Shape/SphereShape.h>
#include <jolt/Physics/Collision/CollideShape.h>
#include "physics/helpers.hpp"

#include <systems/input_helpers.hpp>
#include "systems/collisions.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "rendering/debug_render.hpp"
#include "physics/physics_system.hpp"
#include "physics/debug_renderer.hpp"
#include "math/easing.hpp"
#include "core/ecs.hpp"
#include "displacement/displacer.hpp"
#include "level/level.hpp"
#include "terrain/terrain_collider.hpp"
#include <physics/rigidbody.hpp>
#include <core/transform.hpp>
#include <math/geometry.hpp>
#include <tools/log.hpp>

bool invertZ = false;
bool invertX = false;

void bee::UpdatePlayerControlInversion()
{
    constexpr int defaultGamepad = 0;

    std::optional<int> usedGamepad{};
    if (Engine.Input().IsGamepadAvailable(defaultGamepad) && Engine.Input().IsGamepadActive()) usedGamepad = defaultGamepad;

    //update inverted
    if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::Y) || 
       (usedGamepad && Engine.Input().GetGamepadButtonOnce(usedGamepad.value(), Input::GamepadButton::MenuLeft)))
    {
        invertZ = !invertZ;
    }
    
    /*if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::X) ||
       (usedGamepad && Engine.Input().GetGamepadButtonOnce(usedGamepad.value(), Input::GamepadButton::ShoulderLeft)))
    {
        invertX = !invertX;
    }*/
}

void bee::PlayerMovementSystem(std::shared_ptr<Level> level, float dt)
{
    constexpr float stickRotationSpeed = 1.2f;
    constexpr float mouseSensitivity = 0.0005f;
    constexpr int defaultGamepad = 0;

    std::optional<int> usedGamepad{};
    if (Engine.Input().IsGamepadAvailable(defaultGamepad) && Engine.Input().IsGamepadActive()) usedGamepad = defaultGamepad;

    float tSpeedDelta = 0.0f;
    bool boostActivated = false;
    glm::vec3 inputRotation;

    if (usedGamepad) {
        tSpeedDelta = glm::max(Engine.Input().GetGamepadAxis(usedGamepad.value(), Input::GamepadAxis::TriggerRight), Engine.Input().GetGamepadAxis(usedGamepad.value(), Input::GamepadAxis::TriggerLeft));

        if (Engine.Input().GetGamepadButton(usedGamepad.value(), Input::GamepadButton::ShoulderLeft) || Engine.Input().GetGamepadButton(usedGamepad.value(), Input::GamepadButton::ShoulderRight))
        {
            tSpeedDelta = 1.0f;
        }

        boostActivated = Engine.Input().GetGamepadButtonOnce(usedGamepad.value(), Input::GamepadButton::East)  ||
                         Engine.Input().GetGamepadButtonOnce(usedGamepad.value(), Input::GamepadButton::North) || 
                         Engine.Input().GetGamepadButtonOnce(usedGamepad.value(), Input::GamepadButton::South) ||
                         Engine.Input().GetGamepadButtonOnce(usedGamepad.value(), Input::GamepadButton::West);

        inputRotation = GetInputEulerRotation(usedGamepad, Stick::Left, stickRotationSpeed * dt);
    }
    else {
        if (Engine.Input().GetMouseButton(Input::MouseButton::Left)) 
        {
            tSpeedDelta = 1.0f;
        }
        boostActivated = Engine.Input().GetMouseButtonOnce(Input::MouseButton::Right);
        inputRotation = GetInputEulerRotation(usedGamepad, Stick::Left, mouseSensitivity);
    }

    if (invertZ)
    {
        inputRotation.x *= -1.0f;
    }
    if (invertX)
    {
        inputRotation.z *= -1.0f;
    }

    auto view = Engine.ECS().Registry.view<Transform, Player, RigidBody>();
    for (auto&& [entity, transform, player, rigidbody] : view.each()) {

        float boostFullCycleTime = player.boostActiveTime + player.boostResetTime;

        if (player.boostTimer > glm::min(0.0f, player.boostGracePeriod))
        {
            player.boostTimer -= dt;
        }

        if (player.boostTimer > player.boostGracePeriod)
        {
            boostActivated = false;
        }

        if (boostActivated)
        {
            player.boostTimer = boostFullCycleTime;
        }

        bool boosting = player.boostTimer > player.boostResetTime;
       
        if (tSpeedDelta <= 0.0f)
        {
            tSpeedDelta = player.decceleration;
        }
        else
        {
            tSpeedDelta *= player.acceleration;
        }

        if (boosting)
        {
            tSpeedDelta = player.boostedAcc;
        }

        float tMaxSpeed = glm::clamp(player.boostTimer, 0.0f, player.boostResetTime) / player.boostResetTime;
        tMaxSpeed = Ease::OutPow(tMaxSpeed, 3);
        float maxSpeed = player.maxSpeed + ((player.boostMaxSpeed - player.maxSpeed) * tMaxSpeed);

        float tSpeed = player.speed / maxSpeed;

        tSpeed = glm::clamp(tSpeed + tSpeedDelta * dt, 0.0f, 1.0f);
        player.speed = maxSpeed * tSpeed;
        float movementSpeed = maxSpeed * Ease::OutSine(tSpeed);

        // Collision stuff
        {
            JPH::SphereShape sphere = JPH::SphereShape(player.playerCollisionRadius);
            JPH::Mat44 Jtransform = JPH::Mat44::sIdentity();
            Jtransform.SetTranslation(GlmToJolt(transform.GetTranslation()));

            JPH::CollideShapeSettings collideSettings;
            JPH::SpecifiedBroadPhaseLayerFilter filter(BroadPhaseLayers::NON_MOVING);

            BodyCollisionCollector collector;

            Engine.PhysicsSystem()
                .GetNarrowPhaseQuery()
                .CollideShape(&sphere, JPH::Vec3Arg(1.0f, 1.0f, 1.0f), Jtransform, collideSettings, JPH::RVec3Arg(0.0f, 0.0f, 0.0f), collector, filter);

            if (collector.Collided())
            {
                float compoundedMagnitude = 0;
                glm::vec3 compoundedNormal = glm::vec3(0.0f);

                for (int i = 0; i < collector.results.size(); i += 1)
                {
                    auto& collision = collector.results[i];

                    float percent = collision.magnitude / collector.totalMagnitude;

                    compoundedNormal += collision.normal * percent;

                    compoundedMagnitude += collision.magnitude * percent;
                }

                compoundedNormal = glm::normalize(compoundedNormal);

#if defined(JPH_DEBUG_RENDERER) && defined(BEE_EDITOR)

                JPH::Color color = JPH::Color(0, 255, 0, 255);

                if (collector.Collided())
                {
                    color = JPH::Color(255, 0, 0, 255);
                }

                //JPH::DebugRenderer::sInstance->DrawSphere(Jtransform.GetTranslation(), sphere.GetRadius(), color);

                glm::vec3 from = transform.GetTranslation();
                glm::vec3 to = transform.GetTranslation() + compoundedNormal * compoundedMagnitude;

                JPH::DebugRenderer::sInstance->DrawArrow(GlmToJolt(from), GlmToJolt(to), JPH::Color::sRed, 0.1f);

#endif

                glm::vec3 translation = transform.GetTranslation();
                translation += compoundedNormal * compoundedMagnitude;
                transform.SetTranslation(translation);
            }
        }

        glm::vec3 pushDirection(0.0f);
        float pushStrength = 0.0f;

        {
            auto& terrain = level->GetTerrain();

            float offset = 25;
            float halfX = float(terrain.mapSizeX) * 0.5f - offset;
            float halfY = float(terrain.mapSizeY) * 0.5f - offset;
            float heightLimit = 40.0;

            Engine.DebugRenderer().AddLine(DebugCategory::Enum::Gameplay, glm::vec3( halfX,  halfY, 10.0f), glm::vec3(-halfX,  halfY, 10.0f), glm::vec4(1.0f));
            Engine.DebugRenderer().AddLine(DebugCategory::Enum::Gameplay, glm::vec3( halfX,  halfY, 10.0f), glm::vec3( halfX, -halfY, 10.0f), glm::vec4(1.0f));
            Engine.DebugRenderer().AddLine(DebugCategory::Enum::Gameplay, glm::vec3(-halfX,  halfY, 10.0f), glm::vec3(-halfX, -halfY, 10.0f), glm::vec4(1.0f));
            Engine.DebugRenderer().AddLine(DebugCategory::Enum::Gameplay, glm::vec3( halfX, -halfY, 10.0f), glm::vec3(-halfX, -halfY, 10.0f), glm::vec4(1.0f));

            Engine.DebugRenderer().AddLine(DebugCategory::Enum::Gameplay, glm::vec3( halfX,  halfY, heightLimit), glm::vec3(-halfX, -halfY, heightLimit), glm::vec4(1.0f));
            Engine.DebugRenderer().AddLine(DebugCategory::Enum::Gameplay, glm::vec3(-halfX,  halfY, heightLimit), glm::vec3( halfX, -halfY, heightLimit), glm::vec4(1.0f));

            if (transform.Translation.z > heightLimit)
            {
                pushDirection.z = -1.0f;
                pushStrength = 5.0f;
            }

            if (transform.Translation.x > halfX)
            {
                pushDirection.x = -1.0f;
                pushStrength = 5.0f;
            }

            if (transform.Translation.x < -halfX)
            {
                pushDirection.x = 1.0f;
                pushStrength = 5.0f;
            }

            if (transform.Translation.y > halfY)
            {
                pushDirection.y = -1.0f;
                pushStrength = 5.0f;
            }

            if (transform.Translation.y < -halfY)
            {
                pushDirection.y = 1.0f;
                pushStrength = 5.0f;
            }
        }

        player.pullEulerRadians += inputRotation * player.turnSpeed;
        player.pullEulerRadians.x = glm::clamp(player.pullEulerRadians.x, glm::radians(-89.0f), glm::radians(89.0f));
        glm::vec3 eulerDelta = player.pullEulerRadians - player.eulerRadians;

        float magnitude = glm::length(eulerDelta);
        float t = magnitude / player.maxAngleDelta;
            
        player.eulerRadians += eulerDelta * glm::min(Ease::InPow(t, 2) * dt, 1.0f);

        eulerDelta = player.eulerRadians - player.pullEulerRadians;
        player.pullEulerRadians += eulerDelta * Ease::OutPow(glm::min(player.turnResetModifier * dt, 1.0f), 2);

        player.eulerRadians.x = glm::clamp(player.eulerRadians.x, glm::radians(-89.0f), glm::radians(89.0f));
        glm::quat rotation = glm::quat(player.pullEulerRadians);

        if (pushStrength != 0.0f)
        {
            rigidbody.acceleration += pushDirection * pushStrength;
        }
        else
        {
            rigidbody.acceleration += rotation * World::FORWARD * movementSpeed;
        }

        transform.SetRotation(rotation * World::YUP_TO_ZUP);
    }
}

