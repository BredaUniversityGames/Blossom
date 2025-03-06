#include "precompiled/game_precompiled.hpp"
#include <systems/collisions.hpp>

#include <core/engine.hpp>
#include <physics/physics_system.hpp>
#include <core/ecs.hpp>
#include <core/transform.hpp>
#include <systems/player.hpp>
#include <physics/helpers.hpp>
#include <physics/debug_renderer.hpp>
#include <terrain/terrain_collider.hpp>
#include <physics/rigidbody.hpp>
#include <displacement/displacer.hpp>
#include <math/geometry.hpp>

#include <jolt/Jolt.h>
#include <jolt/Physics/Collision/CollideShape.h>
#include <jolt/Physics/Collision/Shape/BoxShape.h>
#include <jolt/Physics/Collision/Shape/SphereShape.h>
#include <jolt/Physics/Body/BodyCreationSettings.h>
#include <jolt/Physics/Collision/CollisionCollector.h>

void bee::CollectableCollisionCollector::AddHit(const ResultType& inResult)
{
    collidedBodies.push_back(inResult.mBodyID2);
}

bool bee::CollectableCollisionCollector::Collided()
{
    return collidedBodies.size() > 0;
}

void bee::BodyCollisionCollector::AddHit(const ResultType& inResult)
{
    glm::vec3 delta = JoltToGlm(inResult.mContactPointOn2 - inResult.mContactPointOn1);

    Data data;

    data.magnitude = glm::length(delta);

    if (data.magnitude == 0.0f)
    {
        return;
    }

    data.normal = delta / data.magnitude;

    totalMagnitude += data.magnitude;

    results.push_back(data);
}

bool bee::BodyCollisionCollector::Collided()
{
    return results.size() > 0;
}

void bee::PhysicsTest()
{
    //Log::Info("{}", Engine.PhysicsSystem().GetNumActiveBodies(JPH::EBodyType::RigidBody));

    auto players = Engine.ECS().Registry.view<Transform, Player>();

    for (auto&& [entity, transform, player] : players.each())
    {
        JPH::SphereShape sphere = JPH::SphereShape(0.5f);
        JPH::Mat44 Jtransform = JPH::Mat44::sIdentity();
        Jtransform.SetTranslation(GlmToJolt(transform.GetTranslation()));

        JPH::CollideShapeSettings collideSettings;

        CollectableCollisionCollector collector;

        Engine.PhysicsSystem()
            .GetNarrowPhaseQuery()
            .CollideShape(&sphere, JPH::Vec3Arg(1.0f, 1.0f, 1.0f), Jtransform, collideSettings, JPH::RVec3Arg(0.0f, 0.0f, 0.0f), collector);

        JPH::Color color = JPH::Color(0.0f, 255.0f, 0.0f, 255.0f);

        if (collector.Collided())
        {
            color = JPH::Color(255.0f, 0.0f, 0.0f, 255.0f);
        }

        JPH::BodyInterface& bodyInterface = Engine.PhysicsSystem().GetBodyInterface();

        // deletes props on collision
        //for (int i = 0; i < collector.collidedBodies.size(); i += 1)
        //{
        //    entt::entity entity = static_cast<entt::entity>(bodyInterface.GetUserData(collector.collidedBodies[i]));
        //
        //    Engine.ECS().DeleteEntity(entity);
        //}

        // This will be fixed later
        // at the moment using debug renderer on release and 
        // playstation creates a compilation error because of how Jolt is implemented
        EDITOR_ONLY(JPH::DebugRenderer::sInstance->DrawSphere(Jtransform.GetTranslation(), sphere.GetRadius(), color));
    }
}

void bee::TerrainCollisionHandlingSystem(std::shared_ptr<Level> level, float dt)
{
    constexpr float marginFromGround = 0.5f;

    if (level == nullptr) return;

    auto view = Engine.ECS().Registry.view<Transform, RigidBody, Displacer>();
    for (auto&& [entity, transform, rigidbody, displacer] : view.each())
    {
        float terrainHeight = level->GetTerrainCollider().SampleHeightInWorld(transform.GetTranslation()) + marginFromGround;
        const float pushPadding = 0.15f;

        float distanceToTerrain = transform.GetTranslation().z - terrainHeight;
        if (transform.GetTranslation().z < terrainHeight + pushPadding)
        {
            glm::vec3 translation = transform.GetTranslation();

            // Clamp player to terrain height
            if (translation.z < terrainHeight)
            {
                translation.z = terrainHeight;
                transform.SetTranslation(translation);
            }

            rigidbody.acceleration += distanceToTerrain * World::UP * dt;
        }

        float heightMuliplier = 1.0f - distanceToTerrain / 2.0f;
        float speedMultiplier = glm::length(rigidbody.velocity);

        displacer.radius = speedMultiplier * heightMuliplier * 0.3f;
    }
}
